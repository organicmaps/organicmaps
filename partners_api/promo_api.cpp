#include "partners_api/promo_api.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/url_helpers.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>
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

void ParseCityGallery(std::string const & src, UTM utm, promo::CityGallery & result)
{
  base::Json root(src.c_str());
  auto const dataArray = json_object_get(root.get(), "data");

  auto const size = json_array_size(dataArray);

  result.m_items.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    promo::CityGallery::Item item;
    auto const obj = json_array_get(dataArray, i);
    FromJSONObject(obj, "name", item.m_name);
    FromJSONObject(obj, "url", item.m_url);
    item.m_url = InjectUTM(base::url::Join(BOOKMARKS_CATALOG_FRONT_URL, item.m_url), utm);
    FromJSONObject(obj, "access", item.m_access);
    FromJSONObjectOptionalField(obj, "image_url", item.m_imageUrl);
    FromJSONObjectOptionalField(obj, "tier", item.m_tier);

    auto const authorObj = json_object_get(obj, "author");
    FromJSONObject(authorObj, "key_id", item.m_author.m_id);
    FromJSONObject(authorObj, "name", item.m_author.m_name);

    auto const luxCategoryObj = json_object_get(obj, "lux_category");
    if (json_is_object(luxCategoryObj))
    {
      FromJSONObjectOptionalField(luxCategoryObj, "name", item.m_luxCategory.m_name);
      FromJSONObjectOptionalField(luxCategoryObj, "color", item.m_luxCategory.m_color);
    }

    result.m_items.emplace_back(std::move(item));
  }

  auto const meta = json_object_get(root.get(), "meta");
  FromJSONObject(meta, "more", result.m_moreUrl);
  result.m_moreUrl = InjectUTM(base::url::Join(BOOKMARKS_CATALOG_FRONT_URL, result.m_moreUrl), utm);
}

std::string ToSignedId(std::string const & id)
{
  uint64_t unsignedId;
  if (!strings::to_uint64(id, unsignedId))
    unsignedId = 0;

  return strings::to_string(static_cast<int64_t>(unsignedId));
}

std::string MakeCityGalleryUrl(std::string const & baseUrl, std::string const & id,
                               std::string const & lang)
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT_EQUAL(baseUrl.back(), '/', ());

  return baseUrl + "gallery/v1/city/" + ToSignedId(id) + "/?lang=" + lang;
}

std::string GetPictureUrl(std::string const & baseUrl, std::string const & id)
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT_EQUAL(baseUrl.back(), '/', ());

  return baseUrl + "bookmarks_catalogue/city/" + ToSignedId(id) + ".jpg";
}

std::string GetCityCatalogueUrl(std::string const & baseUrl, std::string const & id)
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT_EQUAL(baseUrl.back(), '/', ());

  return baseUrl + "v2/mobilefront/city/" + ToSignedId(id);
}


void GetPromoCityGalleryImpl(std::string const & baseUrl, std::string const & id,
                             std::string const & lang, UTM utm,
                             CityGalleryCallback const & onSuccess,
                             OnError const & onError)
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT_EQUAL(baseUrl.back(), '/', ());

  if (id.empty())
  {
    onSuccess({});
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [baseUrl, id, lang, utm, onSuccess, onError]()
  {
    ASSERT(!id.empty(), ());

    CityGallery result;
    std::string httpResult;
    if (!WebApi::GetCityGalleryById(baseUrl, id, lang, httpResult))
    {
      onError();
      return;
    }

    try
    {
      ParseCityGallery(httpResult, utm, result);
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      onError();
      return;
    }

    onSuccess(std::move(result));
  });
}

std::string LoadPromoIdForBooking(eye::Eye::InfoType const & eyeInfo)
{
  std::string bookingPromoAwaitingForId;
  settings::TryGet("BookingPromoAwaitingForId", bookingPromoAwaitingForId);

  if (bookingPromoAwaitingForId.empty())
    return bookingPromoAwaitingForId;

  auto const timeSinceLastTransitionToBooking =
    eye::Clock::now() - eyeInfo->m_promo.m_transitionToBookingTime;

  if (timeSinceLastTransitionToBooking < kMinMinutesAfterBooking ||
      timeSinceLastTransitionToBooking > kMaxMinutesAfterBooking)
  {
    settings::Delete("BookingPromoAwaitingForId");
    bookingPromoAwaitingForId.clear();
  }

  return bookingPromoAwaitingForId;
}
}  // namespace

// static
bool WebApi::GetCityGalleryById(std::string const & baseUrl, std::string const & id,
                                std::string const & lang, std::string & result)
{
  platform::HttpClient request(MakeCityGalleryUrl(baseUrl, id, lang));
  return request.RunHttpRequest(result);
}

Api::Api(std::string const & baseUrl /* = BOOKMARKS_CATALOG_FRONT_URL */,
         std::string const & basePicturesUrl /* = PICTURES_URL */)
  : m_baseUrl(baseUrl)
  , m_basePicturesUrl(basePicturesUrl)
{
}

void Api::SetDelegate(std::unique_ptr<Delegate> delegate)
{
  m_delegate = std::move(delegate);
}

AfterBooking Api::GetAfterBooking(std::string const & lang) const
{
  auto const eyeInfo = eye::Eye::Instance().GetInfo();

  auto const promoId = LoadPromoIdForBooking(eyeInfo);

  if (!NeedToShowImpl(promoId, eyeInfo))
    return {};

  return {promoId, InjectUTM(GetCityCatalogueUrl(m_baseUrl, promoId), UTM::BookingPromo),
          GetPictureUrl(m_basePicturesUrl, promoId)};
}

std::string Api::GetLinkForDownloader(std::string const & id) const
{
  return InjectUTM(GetCityCatalogueUrl(m_baseUrl, id), UTM::DownloadMwmBanner);
}

std::string Api::GetCityUrl(m2::PointD const & point) const
{
  auto const id = m_delegate->GetCityId(point);

  if (id.empty())
    return {};

  return GetCityCatalogueUrl(m_baseUrl, id);
}

void Api::GetCityGallery(m2::PointD const & point, std::string const & lang, UTM utm,
                         CityGalleryCallback const & onSuccess, OnError const & onError) const
{
  CHECK(m_delegate, ());

  GetPromoCityGalleryImpl(m_baseUrl, m_delegate->GetCityId(point), lang, utm, onSuccess, onError);
}

void Api::OnTransitionToBooking(m2::PointD const & hotelPos)
{
  auto const id = m_delegate->GetCityId(hotelPos);

  if (!id.empty())
    settings::Set("BookingPromoAwaitingForId", id);
}
}  // namespace promo
