#include "partners_api/promo_api.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace std::chrono;

namespace promo
{
namespace
{
constexpr minutes kMinMinutesAfterBooking = minutes(5);
constexpr minutes kMaxMinutesAfterBooking = minutes(60);
constexpr hours kShowPromoNotRaterThan = hours(24);

bool NeedToShowImpl(std::string const & bookingPromoAwaitingForId, eye::Eye::InfoType const & eyeInfo)
{
  if (bookingPromoAwaitingForId.empty() ||
      bookingPromoAwaitingForId == eyeInfo->m_promo.m_lastTimeShownAfterBookingCityId)
  {
    return false;
  }

  auto const timeSinceLastShown = eye::Clock::now() - eyeInfo->m_promo.m_lastTimeShownAfterBooking;
  auto const timeSinceLastTransitionToBooking =
      eye::Clock::now() - eyeInfo->m_promo.m_transitionToBookingTime;

  return timeSinceLastTransitionToBooking >= kMinMinutesAfterBooking &&
         timeSinceLastTransitionToBooking <= kMaxMinutesAfterBooking &&
         timeSinceLastShown > kShowPromoNotRaterThan;
}

void ParseCityGallery(std::string const & src, promo::CityGallery & result)
{
  base::Json root(src.c_str());
  auto const dataArray = json_object_get(root.get(), "data");

  auto const size = json_array_size(dataArray);

  result.m_items.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    promo::CityGalleryItem item;
    auto const obj = json_array_get(dataArray, i);
    FromJSONObject(obj, "name", item.m_name);
    FromJSONObject(obj, "url", item.m_url);

    auto const imageUrlObj = json_object_get(obj, "image_url");
    if (!json_is_null(imageUrlObj))
      FromJSON(imageUrlObj, item.m_imageUrl);

    FromJSONObject(obj, "access", item.m_access);

    auto const tierObj = json_object_get(obj, "tier");
    if (!json_is_null(tierObj))
      FromJSON(tierObj, item.m_tier);

    auto const authorObj = json_object_get(obj, "author");
    FromJSONObject(authorObj, "key_id", item.m_author.m_id);
    FromJSONObject(authorObj, "name", item.m_author.m_name);

    auto const luxCategoryObj = json_object_get(obj, "lux_category");
    if (!json_is_null(luxCategoryObj))
    {
      auto const luxCategoryNameobj = json_object_get(luxCategoryObj, "name");
      if (!json_is_null(luxCategoryNameobj))
        FromJSON(luxCategoryNameobj, item.m_luxCategory.m_name);

      FromJSONObject(luxCategoryObj, "color", item.m_luxCategory.m_color);
    }

    result.m_items.emplace_back(std::move(item));
  }

  auto const meta = json_object_get(root.get(), "meta");
  FromJSONObject(meta, "more", result.m_moreUrl);
}

std::string MakeCityGalleryUrl(std::string const & baseUrl, std::string const & id,
                               std::string const & lang)
{
  return baseUrl + id + "/?lang=" + lang;
}

void GetPromoCityGalleryImpl(std::string const & baseUrl, std::string const & id,
                             CityGalleryCallback const & cb)
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT_EQUAL(baseUrl.back(), '/', ());

  CityGallery result;
  std::string httpResult;
  if (id.empty() || !WebApi::GetCityGalleryById(baseUrl, id, languages::GetCurrentNorm(), httpResult))
  {
    cb({});
    return;
  }

  try
  {
    ParseCityGallery(httpResult, result);
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    result.m_items.clear();
  }

  cb(std::move(result));
}
}  // namespace

// static
bool WebApi::GetCityGalleryById(std::string const & baseUrl, std::string const & id,
                                std::string const & lang, std::string & result)
{
  platform::HttpClient request(MakeCityGalleryUrl(baseUrl, id, lang));
  return request.RunHttpRequest(result);
}

Api::Api(std::string const & baseUrl /* = "https://routes.maps.me/gallery/v1/city/" */)
  : m_baseUrl(baseUrl)
{
}

void Api::SetDelegate(std::unique_ptr<Delegate> delegate)
{
  m_delegate = std::move(delegate);
}

void Api::OnEnterForeground()
{
  m_bookingPromoAwaitingForId.clear();
  settings::TryGet("BookingPromoAwaitingForId", m_bookingPromoAwaitingForId);

  if (m_bookingPromoAwaitingForId.empty())
    return;

  auto const eyeInfo = eye::Eye::Instance().GetInfo();
  auto const timeSinceLastTransitionToBooking =
      eye::Clock::now() - eyeInfo->m_promo.m_transitionToBookingTime;

  if (timeSinceLastTransitionToBooking < kMinMinutesAfterBooking ||
      timeSinceLastTransitionToBooking > kMaxMinutesAfterBooking)
  {
    settings::Delete("BookingPromoAwaitingForId");
    m_bookingPromoAwaitingForId.clear();
  }
}

bool Api::NeedToShowAfterBooking() const
{
  return NeedToShowImpl(m_bookingPromoAwaitingForId, eye::Eye::Instance().GetInfo());
}

std::string Api::GetPromoLinkAfterBooking() const
{
  auto const eyeInfo = eye::Eye::Instance().GetInfo();

  if (!NeedToShowImpl(m_bookingPromoAwaitingForId, eyeInfo))
    return "";

  return MakeCityGalleryUrl(m_baseUrl, m_bookingPromoAwaitingForId, languages::GetCurrentNorm());
}

void Api::GetCityGallery(std::string const & id, CityGalleryCallback const & cb) const
{
  GetPromoCityGalleryImpl(m_baseUrl, id, cb);
}

void Api::GetCityGallery(m2::PointD const & point, CityGalleryCallback const & cb) const
{
  CHECK(m_delegate, ());

  GetPromoCityGalleryImpl(m_baseUrl, m_delegate->GetCityId(point), cb);
}

void Api::OnMapObjectEvent(eye::MapObject const & mapObject)
{
  CHECK(!mapObject.GetEvents().empty(), ());

  auto const bestType = classif().GetTypeByReadableObjectName(mapObject.GetBestType());

  if (!ftypes::IsHotelChecker::Instance()(bestType) &&
      !ftypes::IsBookingHotelChecker::Instance()(bestType))
  {
    return;
  }

  m2::PointD pos;
  bool found = false;
  switch (mapObject.GetEvents().back().m_type)
  {
  case eye::MapObject::Event::Type::BookingBook:
  case eye::MapObject::Event::Type::BookingMore:
  case eye::MapObject::Event::Type::BookingReviews:
  case eye::MapObject::Event::Type::BookingDetails:
  {
    pos = mapObject.GetPos();
    found = true;
  }
  default: /* do nothing */;
  }

  auto const id = found ? m_delegate->GetCityId(pos) : "";

  if (!id.empty())
    settings::Set("BookingPromoAwaitingForId", id);
}
}  // namespace promo
