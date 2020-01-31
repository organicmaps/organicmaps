#include "partners_api/promo_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace base;

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

void ParseCityGallery(std::string const & src, UTM utm, std::string const & utmTerm,
                      promo::CityGallery & result)
{
  Json root(src.c_str());
  auto const dataArray = json_object_get(root.get(), "data");

  auto const size = json_array_size(dataArray);

  result.m_items.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    promo::CityGallery::Item item;
    auto const obj = json_array_get(dataArray, i);
    FromJSONObject(obj, "name", item.m_name);
    FromJSONObject(obj, "url", item.m_url);
    item.m_url = InjectUTM(url::Join(BOOKMARKS_CATALOG_FRONT_URL, item.m_url), utm);
    if (!utmTerm.empty())
      item.m_url = InjectUTMTerm(item.m_url, utmTerm);

    FromJSONObject(obj, "access", item.m_access);
    FromJSONObjectOptionalField(obj, "image_url", item.m_imageUrl);
    FromJSONObjectOptionalField(obj, "tier", item.m_tier);
    FromJSONObjectOptionalField(obj, "tour_category", item.m_tourCategory);

    auto const placeObj = json_object_get(obj, "place");
    if (json_is_object(placeObj))
    {
      FromJSONObject(placeObj, "name", item.m_place.m_name);
      FromJSONObject(placeObj, "description", item.m_place.m_description);
    }

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
  FromJSONObjectOptionalField(meta, "more", result.m_moreUrl);
  result.m_moreUrl = InjectUTM(url::Join(BOOKMARKS_CATALOG_FRONT_URL, result.m_moreUrl), utm);
  if (!utmTerm.empty())
    result.m_moreUrl = InjectUTMTerm(result.m_moreUrl, utmTerm);
  FromJSONObjectOptionalField(meta, "category", result.m_category);
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
  // Support empty baseUrl for opensource build.
  if (id.empty() || baseUrl.empty())
    return {};

  ASSERT_EQUAL(baseUrl.back(), '/', ());

  url::Params params = {{"city_id", ToSignedId(id)}, {"lang", lang}};
  return url::Make(url::Join(baseUrl, "gallery/v2/search/"), params);
}

std::string MakePoiGalleryUrl(std::string const & baseUrl, std::string const & id,
                              m2::PointD const & point, std::string const & lang,
                              std::vector<std::string> const & tags, bool useCoordinates)
{
  // Support opensource build.
  if (baseUrl.empty())
    return {};
    
  url::Params params;

  if (!id.empty())
    params.emplace_back("city_id", ToSignedId(id));

  if (id.empty() || useCoordinates)
  {
    auto const latLon = mercator::ToLatLon(point);
    std::ostringstream os;
    os << std::fixed << std::setprecision(6) << latLon.m_lat << "," << latLon.m_lon;
    params.emplace_back("lat_lon", os.str());
  }

  params.emplace_back("tags", strings::JoinStrings(tags, ","));
  params.emplace_back("lang", lang);

  return url::Make(url::Join(baseUrl, "gallery/v2/search/"), params);
}

std::string GetPictureUrl(std::string const & baseUrl, std::string const & id)
{
  // Support opensource build.
  if (baseUrl.empty())
    return {};

  ASSERT_EQUAL(baseUrl.back(), '/', ());

  return baseUrl + "bookmarks_catalogue/city/" + ToSignedId(id) + ".jpg";
}

std::string GetCityCatalogueUrl(std::string const & baseUrl, std::string const & id)
{
  // Support opensource build.
  if (baseUrl.empty())
    return {};

  ASSERT_EQUAL(baseUrl.back(), '/', ());

  return baseUrl + "v3/mobilefront/city/" + ToSignedId(id);
}

void GetPromoGalleryImpl(std::string const & url, platform::HttpClient::Headers const & headers,
                         UTM utm, std::string const & utmTerm,
                         CityGalleryCallback const & onSuccess, OnError const & onError)
{
  if (url.empty())
  {
    onSuccess({});
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [url, headers, utm, utmTerm, onSuccess, onError]()
  {
    CityGallery result;
    std::string httpResult;
    platform::HttpClient request(url);
    request.SetTimeout(5 /* timeoutSec */);
    request.SetRawHeaders(headers);
    if (!request.RunHttpRequest(httpResult))
    {
      onError();
      return;
    }

    try
    {
      ParseCityGallery(httpResult, utm, utmTerm, result);
    }
    catch (Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg(), httpResult));
      onError();
      return;
    }

    onSuccess(result.IsEmpty() ? CityGallery{} : std::move(result));
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
  auto const cityId = m_delegate->GetCityId(point);
  auto const url = MakeCityGalleryUrl(m_baseUrl, cityId, lang);
  auto const headers = m_delegate->GetHeaders();
  GetPromoGalleryImpl(url, headers, utm, cityId, onSuccess, onError);
}

void Api::GetPoiGallery(m2::PointD const & point, std::string const & lang, Tags const & tags,
                        bool useCoordinates, UTM utm, CityGalleryCallback const & onSuccess,
                        OnError const & onError) const
{
  CHECK(m_delegate, ());

  auto const url =
      MakePoiGalleryUrl(m_baseUrl, m_delegate->GetCityId(point), point, lang, tags, useCoordinates);
  auto const headers = m_delegate->GetHeaders();
  GetPromoGalleryImpl(url, headers, utm, "", onSuccess, onError);
}

void Api::OnTransitionToBooking(m2::PointD const & hotelPos)
{
  auto const id = m_delegate->GetCityId(hotelPos);

  if (!id.empty())
    settings::Set("BookingPromoAwaitingForId", id);
}
}  // namespace promo
