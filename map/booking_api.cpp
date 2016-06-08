#include "map/booking_api.hpp"

#include "platform/http_request.hpp"

#include "base/gmtime.hpp"
#include "base/logging.hpp"

#include "std/chrono.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

char const BookingApi::kDefaultCurrency[1];

BookingApi::BookingApi() : m_affiliateId(BOOKING_AFFILIATE_ID)
{
  stringstream ss;
  ss << BOOKING_KEY << ":" << BOOKING_SECRET;
  m_apiUrl = "https://" + ss.str() + "@distribution-xml.booking.com/json/bookings.";
}
string BookingApi::GetBookingUrl(string const & baseUrl, string const & /* lang */) const
{
  return GetDescriptionUrl(baseUrl) + "#availability";
}

string BookingApi::GetDescriptionUrl(string const & baseUrl, string const & /* lang */) const
{
  return baseUrl + "?affiliate_id=" + m_affiliateId;
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
  auto const callback = [fn, currency](downloader::HttpRequest & answer)
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
  };

  downloader::HttpRequest::Get(url, callback);
}

string BookingApi::MakeApiUrl(string const & func,
                              initializer_list<pair<string, string>> const & params)
{
  stringstream ss;
  ss << m_apiUrl << func << "?";
  bool firstRun = true;
  for (auto const & param : params)
    ss << (firstRun ? firstRun = false, "" : "&") << param.first << "=" << param.second;

  return ss.str();
}