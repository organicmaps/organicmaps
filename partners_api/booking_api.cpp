#include "partners_api/booking_api.hpp"
#include "partners_api/utils.hpp"

#include "platform/http_client.hpp"
#include "platform/locale.hpp"
#include "platform/platform.hpp"

#include "coding/url.hpp"
#include "coding/sha1.hpp"

#include "base/get_time.hpp"
#include "base/logging.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <sstream>
#include <utility>

#include "3party/jansson/myjansson.hpp"

#include "private.h"

using namespace base;
using namespace booking;
using namespace platform;
using namespace std;
using namespace std::chrono;

namespace
{
string const kBookingApiBaseUrlV1 = "https://distribution-xml.booking.com/json/bookings";
string const kBookingApiBaseUrlV2 = "https://distribution-xml.booking.com/2.6/json";
string const kExtendedHotelInfoBaseUrl = BOOKING_EXTENDED_INFO_BASE_URL;
string const kPhotoOriginalUrl = "http://aff.bstatic.com/images/hotel/max500/";
string const kPhotoSmallUrl = "http://aff.bstatic.com/images/hotel/max300/";
string const kSearchBaseUrl = "https://www.booking.com/search.html";
string const kDeepLinkBaseUrl = "booking://hotel/";
string g_BookingUrlForTesting = "";

booking::AvailabilityParams::UrlFilter const kAvailabilityParamsForUniversalLink =
{
  "checkin",
  "checkout",
  "room"
};
booking::AvailabilityParams::UrlFilter const kAvailabilityParamsForDeepLink =
{
  "checkin",
  "checkout"
};

bool RunSimpleHttpRequest(bool const needAuth, string const & url, string & result)
{
  HttpClient request(url);

  if (needAuth)
    request.SetUserAndPassword(BOOKING_KEY, BOOKING_SECRET);

  return request.RunHttpRequest(result);
}

string MakeUrlForTesting(string const & func, url::Params const & params, string const & divider)
{
  ASSERT(!g_BookingUrlForTesting.empty(), ());

  auto funcForTesting = func;
  if (funcForTesting == "hotelAvailability")
  {
    auto const it = find_if(params.cbegin(), params.cend(), [](url::Param const & param)
    {
      return param.m_name == "show_only_deals";
    });

    if (it != params.cend())
      funcForTesting = "deals";
  }

  return url::Make(g_BookingUrlForTesting + divider + funcForTesting, params);
}

string MakeApiUrlImpl(string const & baseUrl, string const & func, url::Params const & params,
                      string const & divider)
{
  if (!g_BookingUrlForTesting.empty())
    return MakeUrlForTesting(func, params, divider);

  return url::Make(baseUrl + divider + func, params);
}

string MakeApiUrlV2(string const & func, url::Params const & params)
{
  return MakeApiUrlImpl(kBookingApiBaseUrlV2, func, params, "/");
}

void ClearHotelInfo(HotelInfo & info)
{
  info.m_hotelId.clear();
  info.m_description.clear();
  info.m_photos.clear();
  info.m_facilities.clear();
  info.m_reviews.clear();
  info.m_score = 0.0;
  info.m_scoreCount = 0;
}

vector<HotelFacility> ParseFacilities(json_t const * facilitiesArray)
{
  vector<HotelFacility> facilities;

  if (facilitiesArray == nullptr || !json_is_array(facilitiesArray))
    return facilities;

  size_t sz = json_array_size(facilitiesArray);

  for (size_t i = 0; i < sz; ++i)
  {
    auto itemArray = json_array_get(facilitiesArray, i);
    ASSERT(json_is_array(itemArray), ());
    ASSERT_EQUAL(json_array_size(itemArray), 2, ());

    HotelFacility facility;
    FromJSON(json_array_get(itemArray, 0), facility.m_type);
    FromJSON(json_array_get(itemArray, 1), facility.m_name);

    facilities.push_back(move(facility));
  }

  return facilities;
}

vector<HotelPhotoUrls> ParsePhotos(json_t const * photosArray)
{
  if (photosArray == nullptr || !json_is_array(photosArray))
    return {};

  vector<HotelPhotoUrls> photos;
  size_t sz = json_array_size(photosArray);
  string photoId;

  for (size_t i = 0; i < sz; ++i)
  {
    auto item = json_array_get(photosArray, i);

    // Sometimes booking.com returns photo ids as strings, sometimes as integers.
    photoId = FromJSONToString(item);

    // First three digits of id are used as part of path to photo on the server.
    if (photoId.size() < 3)
    {
      LOG(LWARNING, ("Incorrect photo id =", photoId));
      continue;
    }

    string url(photoId.substr(0, 3) + "/" + photoId + ".jpg");
    photos.push_back({kPhotoSmallUrl + url, kPhotoOriginalUrl + url});
  }

  return photos;
}

vector<HotelReview> ParseReviews(json_t const * reviewsArray)
{
  if (reviewsArray == nullptr || !json_is_array(reviewsArray))
    return {};

  vector<HotelReview> reviews;
  size_t sz = json_array_size(reviewsArray);
  string date;

  for (size_t i = 0; i < sz; ++i)
  {
    auto item = json_array_get(reviewsArray, i);
    HotelReview review;

    FromJSONObject(item, "date", date);
    istringstream ss(date);
    tm t = {};
    ss >> base::get_time(&t, "%Y-%m-%d %H:%M:%S");
    if (ss.fail())
    {
      LOG(LWARNING, ("Incorrect review date =", date));
      continue;
    }
    review.m_date = system_clock::from_time_t(mktime(&t));

    double score;
    FromJSONObject(item, "score", score);
    review.m_score = static_cast<float>(score);

    FromJSONObject(item, "author", review.m_author);
    FromJSONObject(item, "pros", review.m_pros);
    FromJSONObject(item, "cons", review.m_cons);

    reviews.push_back(move(review));
  }

  return reviews;
}

void FillHotelInfo(string const & src, HotelInfo & info)
{
  base::Json root(src.c_str());

  FromJSONObjectOptionalField(root.get(), "description", info.m_description);
  double score;
  FromJSONObjectOptionalField(root.get(), "score", score);
  info.m_score = static_cast<float>(score);

  int64_t scoreCount = 0;
  FromJSONObjectOptionalField(root.get(), "score_count", scoreCount);
  info.m_scoreCount = static_cast<uint32_t>(scoreCount);

  auto const facilitiesArray = json_object_get(root.get(), "facilities");
  info.m_facilities = ParseFacilities(facilitiesArray);

  auto const photosArray = json_object_get(root.get(), "photos");
  info.m_photos = ParsePhotos(photosArray);

  auto const reviewsArray = json_object_get(root.get(), "reviews");
  info.m_reviews = ParseReviews(reviewsArray);
}

void FillPriceAndCurrency(json_t * src, string const & currency, BlockInfo & result)
{
  FromJSONObject(src, "currency", result.m_currency);
  FromJSONObject(src, "price", result.m_minPrice);

  if (currency.empty() || result.m_currency == currency)
    return;

  // Try to get price in requested currency.
  auto other = json_object_get(src, "other_currency");
  if (!json_is_object(other))
    return;

  string otherCurrency;
  FromJSONObject(other, "currency", otherCurrency);
  if (otherCurrency == currency)
  {
    result.m_currency = otherCurrency;
    FromJSONObject(other, "price", result.m_minPrice);
    return;
  }
}

BlockInfo MakeBlock(json_t * src, string const & currency)
{
  BlockInfo result;

  FromJSONObject(src, "block_id", result.m_blockId);
  FromJSONObject(src, "name", result.m_name);
  FromJSONObject(src, "room_description", result.m_description);
  FromJSONObject(src, "max_occupancy", result.m_maxOccupancy);
  FromJSONObject(src, "breakfast_included", result.m_breakfastIncluded);
  FromJSONObject(src, "deposit_required", result.m_depositRequired);

  auto minPriceRoot = json_object_get(src, "min_price");
  if (!json_is_object(minPriceRoot))
    MYTHROW(base::Json::Exception, ("The min_price must contain a json object."));

  FillPriceAndCurrency(minPriceRoot, currency, result);

  auto photosArray = json_object_get(src, "photos");
  size_t sz = json_array_size(photosArray);
  string photoUrl;
  for (size_t i = 0; i < sz; ++i)
  {
    auto photoItem = json_array_get(photosArray, i);
    FromJSONObject(photoItem, "url_original", photoUrl);
    result.m_photos.emplace_back(photoUrl);
  }

  auto & deals = result.m_deals;
  bool lastMinuteDeal = false;
  FromJSONObjectOptionalField(src, "is_last_minute_deal", lastMinuteDeal);
  if (lastMinuteDeal)
  {
    deals.m_types.emplace_back(Deals::Type::LastMinute);
    FromJSONObject(src, "last_minute_deal_percentage", deals.m_discount);
  }
  bool smartDeal = false;
  FromJSONObjectOptionalField(src, "is_smart_deal", smartDeal);
  if (smartDeal)
    deals.m_types.emplace_back(Deals::Type::Smart);

  string refundableUntil;
  auto refundableUntilObject = json_object_get(src, "refundable_until");
  if (json_is_string(refundableUntilObject))
  {
    FromJSON(refundableUntilObject, refundableUntil);

    if (!refundableUntil.empty())
    {
      istringstream ss(refundableUntil);
      tm t = {};
      ss >> base::get_time(&t, "%Y-%m-%d %H:%M:%S");
      if (ss.fail())
        LOG(LWARNING, ("Incorrect refundable_until date =", refundableUntil));
      else
        result.m_refundableUntil = system_clock::from_time_t(mktime(&t));
    }
  }

  return result;
}

void FillBlocks(string const & src, string const & currency, Blocks & blocks)
{
  base::Json root(src.c_str());
  if (!json_is_object(root.get()))
    MYTHROW(base::Json::Exception, ("The answer must contain a json object."));

  auto rootArray = json_object_get(root.get(), "result");
  if (!json_is_array(rootArray))
    MYTHROW(base::Json::Exception, ("The \"result\" field must contain a json array."));

  size_t const rootSize = json_array_size(rootArray);
  ASSERT_LESS(rootSize, 2, ("Several hotels is not supported in this method"));
  if (rootSize == 0)
    return;

  auto rootItem = json_array_get(rootArray, 0);
  if (!json_is_object(rootItem))
    MYTHROW(base::Json::Exception, ("The root item must contain a json object."));

  auto blocksArray = json_object_get(rootItem, "block");
  if (!json_is_array(blocksArray))
    MYTHROW(base::Json::Exception, ("The \"block\" field must contain a json array."));

  size_t const blocksSize = json_array_size(blocksArray);
  for (size_t i = 0; i < blocksSize; ++i)
  {
    auto block = json_array_get(blocksArray, i);

    if (!json_is_object(block))
      MYTHROW(base::Json::Exception, ("The block item must contain a json object."));

    blocks.Add(MakeBlock(block, currency));
  }
}

void FillHotels(string const & src, HotelsWithExtras & hotels)
{
  base::Json root(src.c_str());
  auto const resultsArray = json_object_get(root.get(), "result");

  auto const size = json_array_size(resultsArray);

  hotels.reserve(size);
  for (size_t i = 0; i < size; ++i)
  {
    auto const obj = json_array_get(resultsArray, i);
    uint64_t id = 0;
    Extras extras;
    FromJSONObject(obj, "hotel_id", id);
    FromJSONObject(obj, "price", extras.m_price);
    FromJSONObject(obj, "hotel_currency_code", extras.m_currency);

    hotels.emplace(std::to_string(id), std::move(extras));
  }
}

string ApplyAvailabilityParamsUniversal(string const & url, AvailabilityParams const & params)
{
  auto p = params.Get(kAvailabilityParamsForUniversalLink);

// Booking web-site for android browsers works incorrect without |no_rooms| parameter.
#ifdef OMIM_OS_ANDROID
  p.emplace_back("no_rooms", std::to_string(params.m_orderingParams.m_rooms.size()));
#endif

  auto const pos = url.find('#');

  if (pos == string::npos)
    return url::Make(url, p);

  string result = url::Make(url.substr(0, pos), p);
  result.append(url.substr(pos));
  return result;
}

string ApplyAvailabilityParamsDeep(string const & url, AvailabilityParams const & params)
{
  auto p = params.Get(kAvailabilityParamsForDeepLink);

  auto & rooms = params.m_orderingParams.m_rooms;
  uint32_t sum = 0;
  for (auto const & room : rooms)
  {
    sum += room.GetAdultsCount();
  }

  p.emplace_back("numberOfGuests", std::to_string(sum));

  return url::Make(url, p);
}

string MakeLabel(string const & labelSource)
{
  ASSERT(!labelSource.empty(), ());
  auto static const kDeviceIdHash =
      coding::SHA1::CalculateForStringFormatted(GetPlatform().UniqueClientId());

  return labelSource + "-" + url::UrlEncode(kDeviceIdHash);
}
}  // namespace

namespace booking
{
// static
bool RawApi::GetExtendedInfo(string const & hotelId, string const & lang, string & result)
{
  ostringstream os;
  os << kExtendedHotelInfoBaseUrl << "?hotel_id=" << hotelId << "&lang=" << lang;
  return RunSimpleHttpRequest(false, os.str(), result);
}

// static
bool RawApi::HotelAvailability(AvailabilityParams const & params, string & result)
{
  auto const locale = platform::GetCurrentLocale();
  auto p = params.Get();
  p.emplace_back("guest_country", !locale.m_country.empty() ? locale.m_country : "US");

  string url = MakeApiUrlV2("hotelAvailability", p);

  return RunSimpleHttpRequest(true, url, result);
}

// static
bool RawApi::BlockAvailability(BlockParams const & params, string & result)
{
  auto const locale = platform::GetCurrentLocale();
  auto p = params.Get();
  p.emplace_back("guest_cc", locale.m_country);

  string url = MakeApiUrlV2("blockAvailability", p);

  return RunSimpleHttpRequest(true, url, result);
}

Api::Api()
  : m_affiliateId(BOOKING_AFFILIATE_ID)
{
}

string Api::GetBookHotelUrl(string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  url::Params p = {{"aid", m_affiliateId}, {"label", MakeLabel("ppActionButton")}};
  return url::Make(baseUrl, p);
}

string Api::GetDeepLink(string const & hotelId) const
{
  ASSERT(!hotelId.empty(), ());

  ostringstream os;
  os << kDeepLinkBaseUrl << hotelId << "?affiliate_id=" << m_affiliateId;

  return os.str();
}

string Api::GetDescriptionUrl(string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  url::Params p = {{"aid", m_affiliateId}, {"label", MakeLabel("ppDetails")}};
  return url::Make(baseUrl, p);
}

string Api::GetMoreUrl(string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  url::Params p = {{"aid", m_affiliateId}, {"label", MakeLabel("ppMoreInfo")}};
  return url::Make(baseUrl, p);
}

string Api::GetHotelReviewsUrl(string const & hotelId, string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT(!hotelId.empty(), ());

  url::Params p = {{"aid", m_affiliateId}, {"label", MakeLabel("ppReviews")}, {"tab", "4"}};
  return url::Make(baseUrl, p);
}

string Api::GetSearchUrl(string const & city, string const & name) const
{
  if (city.empty() || name.empty())
    return {};

  ostringstream paramStream;
  paramStream << city << " " << name;

  auto const urlEncodedParams = url::UrlEncode(paramStream.str());

  if (urlEncodedParams.empty())
    return {};

  url::Params p = {{"aid", m_affiliateId},
                   {"label", MakeLabel("ppActionButtonOSM")},
                   {"ss", urlEncodedParams}};
  return url::Make(kSearchBaseUrl, p);
}

string Api::ApplyAvailabilityParams(string const & url, AvailabilityParams const & params) const
{
  ASSERT(!url.empty(), ());

  if (params.IsEmpty())
    return url;

  if (strings::StartsWith(url, "booking"))
    return ApplyAvailabilityParamsDeep(url, params);

  return ApplyAvailabilityParamsUniversal(url, params);
}

void Api::GetBlockAvailability(BlockParams && params,
                               BlockAvailabilityCallback const & fn) const
{
  GetPlatform().RunTask(Platform::Thread::Network, [params = move(params), fn]()
  {
    string httpResult;
    if (!RawApi::BlockAvailability(params, httpResult))
    {
      fn(params.m_hotelId, {});
      return;
    }

    Blocks blocks;
    try
    {
      FillBlocks(httpResult, params.m_currency, blocks);
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      blocks = {};
    }
    fn(params.m_hotelId, blocks);
  });
}

void Api::GetHotelAvailability(AvailabilityParams const & params,
                               GetHotelAvailabilityCallback const & fn) const
{
  GetPlatform().RunTask(Platform::Thread::Network, [params, fn]()
  {
    HotelsWithExtras result;
    string httpResult;
    if (!RawApi::HotelAvailability(params, httpResult))
    {
      fn(std::move(result));
      return;
    }

    try
    {
      FillHotels(httpResult, result);
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      result.clear();
    }

    fn(std::move(result));
  });
}

void Api::GetHotelInfo(string const & hotelId, string const & lang,
                       GetHotelInfoCallback const & fn) const
{
  GetPlatform().RunTask(Platform::Thread::Network, [hotelId, lang, fn]()
  {
    HotelInfo info;
    info.m_hotelId = hotelId;

    string result;
    if (!RawApi::GetExtendedInfo(hotelId, lang, result))
    {
      fn(info);
      return;
    }

    try
    {
      FillHotelInfo(result, info);
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LINFO, ("Failed to parse json:", hotelId, result, e.what()));
      ClearHotelInfo(info);
    }

    fn(info);
  });
}

void Api::SetAffiliateId(string const & affiliateId)
{
  m_affiliateId = affiliateId;
}

void SetBookingUrlForTesting(string const & url)
{
  g_BookingUrlForTesting = url;
}
}  // namespace booking
