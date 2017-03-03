#include "partners_api/booking_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "base/gmtime.hpp"
#include "base/logging.hpp"
#include "base/thread.hpp"

#include "std/initializer_list.hpp"
#include "std/iomanip.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"
#include "std/utility.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

namespace
{
using namespace platform;
using namespace booking;

string const kBookingApiBaseUrl = "https://distribution-xml.booking.com/json/bookings";
string const kExtendedHotelInfoBaseUrl = "https://hotels.maps.me/getDescription";
string const kPhotoOriginalUrl = "http://aff.bstatic.com/images/hotel/max500/";
string const kPhotoSmallUrl = "http://aff.bstatic.com/images/hotel/max300/";
string g_BookingUrlForTesting = "";

bool RunSimpleHttpRequest(bool const needAuth, string const & url, string & result)
{
  HttpClient request(url);

  if (needAuth)
    request.SetUserAndPassword(BOOKING_KEY, BOOKING_SECRET);

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }
  return false;
}

string MakeApiUrl(string const & func, initializer_list<pair<string, string>> const & params)
{
  ASSERT_NOT_EQUAL(params.size(), 0, ());

  ostringstream os;
  if (!g_BookingUrlForTesting.empty())
    os << g_BookingUrlForTesting << "." << func << "?";
  else
    os << kBookingApiBaseUrl << "." << func << "?";

  bool firstParam = true;
  for (auto const & param : params)
  {
    if (firstParam)
    {
      firstParam = false;
    }
    else
    {
      os << "&";
    }
    os << param.first << "=" << param.second;
  }

  return os.str();
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
    my::FromJSON(json_array_get(itemArray, 0), facility.m_type);
    my::FromJSON(json_array_get(itemArray, 1), facility.m_name);

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
    my::FromJSON(item, photoId);

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

    my::FromJSONObject(item, "date", date);
    istringstream ss(date);
    tm t = {};
    ss >> get_time(&t, "%Y-%m-%d %H:%M:%S");
    if (ss.fail())
    {
      LOG(LWARNING, ("Incorrect review date =", date));
      continue;
    }
    review.m_date = system_clock::from_time_t(mktime(&t));

    double score;
    my::FromJSONObject(item, "score", score);
    review.m_score = static_cast<float>(score);

    my::FromJSONObject(item, "author", review.m_author);
    my::FromJSONObject(item, "pros", review.m_pros);
    my::FromJSONObject(item, "cons", review.m_cons);

    reviews.push_back(move(review));
  }

  return reviews;
}

void FillHotelInfo(string const & src, HotelInfo & info)
{
  my::Json root(src.c_str());

  my::FromJSONObjectOptionalField(root.get(), "description", info.m_description);
  double score;
  my::FromJSONObjectOptionalField(root.get(), "score", score);
  info.m_score = static_cast<float>(score);

  json_int_t scoreCount = 0;
  my::FromJSONObjectOptionalField(root.get(), "score_count", scoreCount);
  info.m_scoreCount = static_cast<uint32_t>(scoreCount);

  auto const facilitiesArray = json_object_get(root.get(), "facilities");
  info.m_facilities = ParseFacilities(facilitiesArray);

  auto const photosArray = json_object_get(root.get(), "photos");
  info.m_photos = ParsePhotos(photosArray);

  auto const reviewsArray = json_object_get(root.get(), "reviews");
  info.m_reviews = ParseReviews(reviewsArray);
}

void FillPriceAndCurrency(string const & src, string const & currency, string & minPrice,
                          string & priceCurrency)
{
  my::Json root(src.c_str());
  if (!json_is_array(root.get()))
    MYTHROW(my::Json::Exception, ("The answer must contain a json array."));
  size_t const rootSize = json_array_size(root.get());

  if (rootSize == 0)
    return;

  // Read default hotel price and currency.
  auto obj = json_array_get(root.get(), 0);
  my::FromJSONObject(obj, "min_price", minPrice);
  my::FromJSONObject(obj, "currency_code", priceCurrency);

  if (currency.empty() || priceCurrency == currency)
    return;

  // Try to get price in requested currency.
  json_t * arr = json_object_get(obj, "other_currency");
  if (arr == nullptr || !json_is_array(arr))
    return;

  size_t sz = json_array_size(arr);
  string code;
  for (size_t i = 0; i < sz; ++i)
  {
    auto el = json_array_get(arr, i);
    my::FromJSONObject(el, "currency_code", code);
    if (code == currency)
    {
      priceCurrency = code;
      my::FromJSONObject(el, "min_price", minPrice);
      break;
    }
  }
}
}  // namespace

namespace booking
{
// static
bool RawApi::GetHotelAvailability(string const & hotelId, string const & currency, string & result)
{
  char dateArrival[12]{};
  char dateDeparture[12]{};

  system_clock::time_point p = system_clock::from_time_t(time(nullptr));
  tm arrival = my::GmTime(system_clock::to_time_t(p));
  tm departure = my::GmTime(system_clock::to_time_t(p + hours(24)));
  strftime(dateArrival, sizeof(dateArrival), "%Y-%m-%d", &arrival);
  strftime(dateDeparture, sizeof(dateDeparture), "%Y-%m-%d", &departure);

  string url = MakeApiUrl("getHotelAvailability", {{"hotel_ids", hotelId},
                                                   {"currency_code", currency},
                                                   {"arrival_date", dateArrival},
                                                   {"departure_date", dateDeparture}});
  return RunSimpleHttpRequest(true, url, result);
}

// static
bool RawApi::GetExtendedInfo(string const & hotelId, string const & lang, string & result)
{
  ostringstream os;
  os << kExtendedHotelInfoBaseUrl << "?hotel_id=" << hotelId << "&lang=" << lang;
  return RunSimpleHttpRequest(false, os.str(), result);
}

string Api::GetBookHotelUrl(string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  return GetDescriptionUrl(baseUrl) + "#availability";
}

string Api::GetDescriptionUrl(string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  return baseUrl + string("?aid=") + BOOKING_AFFILIATE_ID;
}

string Api::GetHotelReviewsUrl(string const & hotelId, string const & baseUrl) const
{
  ASSERT(!baseUrl.empty(), ());
  ASSERT(!hotelId.empty(), ());
  ostringstream os;
  os << GetDescriptionUrl(baseUrl) << "&tab=4&label=hotel-" << hotelId << "_reviews";
  return os.str();
}

void Api::GetMinPrice(string const & hotelId, string const & currency,
                      GetMinPriceCallback const & fn)
{
  threads::SimpleThread([hotelId, currency, fn]()
  {
    string minPrice;
    string priceCurrency;
    string httpResult;
    if (!RawApi::GetHotelAvailability(hotelId, currency, httpResult))
    {
      fn(hotelId, minPrice, priceCurrency);
      return;
    }

    try
    {
      FillPriceAndCurrency(httpResult, currency, minPrice, priceCurrency);
    }
    catch (my::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      minPrice.clear();
      priceCurrency.clear();
    }
    fn(hotelId, minPrice, priceCurrency);
  }).detach();
}

void Api::GetHotelInfo(string const & hotelId, string const & lang, GetHotelInfoCallback const & fn)
{
  threads::SimpleThread([hotelId, lang, fn]()
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
    catch (my::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      ClearHotelInfo(info);
    }

    fn(info);
  }).detach();
}

void SetBookingUrlForTesting(string const & url)
{
  g_BookingUrlForTesting = url;
}
}  // namespace booking
