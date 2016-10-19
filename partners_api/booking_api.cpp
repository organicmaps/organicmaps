#include "partners_api/booking_api.hpp"

#include "base/gmtime.hpp"
#include "base/logging.hpp"

#include "std/chrono.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

char const BookingApi::kDefaultCurrency[1];

BookingApi::BookingApi() : m_affiliateId(BOOKING_AFFILIATE_ID), m_testingMode(false)
{
  stringstream ss;
  ss << BOOKING_KEY << ":" << BOOKING_SECRET;
  m_apiUrl = "https://" + ss.str() + "@distribution-xml.booking.com/json/bookings.";
}

string BookingApi::GetBookHotelUrl(string const & baseUrl, string const & /* lang */) const
{
  return GetDescriptionUrl(baseUrl) + "#availability";
}

string BookingApi::GetDescriptionUrl(string const & baseUrl, string const & /* lang */) const
{
  return baseUrl + "?aid=" + m_affiliateId;
}

void BookingApi::GetMinPrice(string const & hotelId, string const & currency,
                             function<void(string const &, string const &)> const & fn)
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
  auto const callback = [this, fn, currency](downloader::HttpRequest & answer)
  {

    string minPrice;
    string priceCurrency;
    try
    {
      my::Json root(answer.Data().c_str());
      if (!json_is_array(root.get()))
        MYTHROW(my::Json::Exception, ("The answer must contain a json array."));
      size_t const sz = json_array_size(root.get());

      if (sz > 0)
      {
        // Read default hotel price and currency.
        auto obj = json_array_get(root.get(), 0);
        my::FromJSONObject(obj, "min_price", minPrice);
        my::FromJSONObject(obj, "currency_code", priceCurrency);

        // Try to get price in requested currency.
        if (!currency.empty() && priceCurrency != currency)
        {
          json_t * arr = json_object_get(obj, "other_currency");
          if (arr && json_is_array(arr))
          {
            size_t sz = json_array_size(arr);
            for (size_t i = 0; i < sz; ++i)
            {
              auto el = json_array_get(arr, i);
              string code;
              my::FromJSONObject(el, "currency_code", code);
              if (code == currency)
              {
                priceCurrency = code;
                my::FromJSONObject(el, "min_price", minPrice);
                break;
              }
            }
          }
        }
      }
    }
    catch (my::Json::Exception const & e)
    {
      LOG(LERROR, (e.Msg()));
      minPrice.clear();
      priceCurrency.clear();
    }
    fn(minPrice, priceCurrency);
    m_request.reset();
  };

  m_request.reset(downloader::HttpRequest::Get(url, callback));
}

// TODO(mgsergio): This is just a mockup, make it a real function.
void BookingApi::GetHotelInfo(string const & hotelId, string const & /* lang */,
                              function<void(HotelInfo const & hotelInfo)> const & fn)
{
  HotelInfo info;

  info.m_hotelId = "000";
  info.m_description = "Interesting place among SoHo, Little "
      "Italy and China town. Modern design. "
      "Great view from roof. Near subway. "
      "Free refreshment every afternoon. "
      "The staff was very friendly.";

  info.m_photos.push_back({
      "http://storage9.static.itmages.ru/i/16/0915/h_1473944906_4427771_63a7c2282b.jpg",
      "http://storage7.static.itmages.ru/i/16/0915/h_1473945189_5545647_db54564f06.jpg"});

  info.m_photos.push_back({
      "http://storage9.static.itmages.ru/i/16/0915/h_1473944906_1573275_450fcd78b0.jpg",
      "http://storage8.static.itmages.ru/i/16/0915/h_1473945194_6402871_b68c63c705.jpg"});

  info.m_photos.push_back({
      "http://storage1.static.itmages.ru/i/16/0915/h_1473944906_6998375_f1ba6024a5.jpg",
      "http://storage7.static.itmages.ru/i/16/0915/h_1473945188_9401486_7185c713bc.jpg"});

  info.m_photos.push_back({
      "http://storage7.static.itmages.ru/i/16/0915/h_1473944904_8294064_035b4328ee.jpg",
      "http://storage9.static.itmages.ru/i/16/0915/h_1473945189_8999398_d9bfe0d56d.jpg"});

  info.m_photos.push_back({
      "http://storage6.static.itmages.ru/i/16/0915/h_1473944904_2231876_680171f67f.jpg",
      "http://storage1.static.itmages.ru/i/16/0915/h_1473945190_2042562_c6cfcccd18.jpg"});

  info.m_photos.push_back({
      "http://storage7.static.itmages.ru/i/16/0915/h_1473944904_2871576_660e0aad58.jpg",
           "http://storage1.static.itmages.ru/i/16/0915/h_1473945190_9605355_94164142b7.jpg"});

  info.m_photos.push_back({
      "http://storage8.static.itmages.ru/i/16/0915/h_1473944905_3578559_d4e95070e9.jpg",
      "http://storage3.static.itmages.ru/i/16/0915/h_1473945190_3367031_145793d530.jpg"});

  info.m_photos.push_back({
      "http://storage8.static.itmages.ru/i/16/0915/h_1473944905_5596402_9bdce96ace.jpg",
      "http://storage4.static.itmages.ru/i/16/0915/h_1473945191_2783367_2440027ece.jpg"});

  info.m_photos.push_back({
      "http://storage8.static.itmages.ru/i/16/0915/h_1473944905_4312757_433c687f4d.jpg",
      "http://storage6.static.itmages.ru/i/16/0915/h_1473945191_1817571_b945aa1f3e.jpg"});

  info.m_facilities = {
    {"non_smoking_rooms", "Non smoking rooms"},
    {"gym", "Training gym"},
    {"pets_are_allowed", "Pets are allowed"}
  };

  info.m_reviews = {
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous1",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_5332083_b44af613bd.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_7504873_be0fe246e3.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage1.static.itmages.ru/i/16/0915/h_1473945374_9397526_996bbca0d7.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous1",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_5332083_b44af613bd.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_7504873_be0fe246e3.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage1.static.itmages.ru/i/16/0915/h_1473945374_9397526_996bbca0d7.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous1",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_5332083_b44af613bd.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_7504873_be0fe246e3.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage1.static.itmages.ru/i/16/0915/h_1473945374_9397526_996bbca0d7.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous1",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_5332083_b44af613bd.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage2.static.itmages.ru/i/16/0915/h_1473945375_7504873_be0fe246e3.jpg",
      9.2,
      system_clock::now()
    ),
    HotelReview::CriticReview(
      "Interesting place among SoHo, Little Italy and China town. Modern design. Great view from roof. Near subway. Free refreshment every afternoon. The staff was very friendly.",
      "Little bit noise from outside",
      "Anonymous2",
      "http://storage1.static.itmages.ru/i/16/0915/h_1473945374_9397526_996bbca0d7.jpg",
      9.2,
      system_clock::now()
   )
  };

  fn(info);
}

string BookingApi::MakeApiUrl(string const & func,
                              initializer_list<pair<string, string>> const & params)
{
  stringstream ss;
  ss << m_apiUrl << func << "?";
  bool firstRun = true;
  for (auto const & param : params)
    ss << (firstRun ? firstRun = false, "" : "&") << param.first << "=" << param.second;
  if (m_testingMode)
    ss << "&show_test=1";

  return ss.str();
}
