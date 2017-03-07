#include "testing/testing.hpp"

#include "partners_api/booking_api.hpp"

#include "base/scope_guard.hpp"

namespace
{
UNIT_TEST(Booking_GetHotelAvailability)
{
  string const kHotelId = "98251";  // Booking hotel id for testing.
  string result;
  TEST(booking::RawApi::GetHotelAvailability(kHotelId, "", result), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Booking_GetExtendedInfo)
{
  string const kHotelId = "0";  // Internal hotel id for testing.
  string result;
  TEST(booking::RawApi::GetExtendedInfo(kHotelId, "en", result), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Booking_GetMinPrice)
{
  booking::SetBookingUrlForTesting("http://localhost:34568/booking/min_price");
  MY_SCOPE_GUARD(cleanup, []() { booking::SetBookingUrlForTesting(""); });

  string const kHotelId = "0";  // Internal hotel id for testing.
  booking::Api api;
  {
    string price;
    string currency;
    string hotelId;
    api.GetMinPrice(kHotelId, "" /* default currency */,
                    [&hotelId, &price, &currency](string const & id, string const & val, string const & curr) {
                      hotelId = id;
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST_EQUAL(hotelId, kHotelId, ());
    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "USD", ());
  }

  {
    string price;
    string currency;
    string hotelId;
    api.GetMinPrice(kHotelId, "RUB", [&hotelId, &price, &currency](string const & id, string const & val, string const & curr)
                    {
                      hotelId = id;
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST_EQUAL(hotelId, kHotelId, ());
    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "RUB", ());
  }

  {
    string price;
    string currency;
    string hotelId;
    api.GetMinPrice(kHotelId, "ISK", [&hotelId, &price, &currency](string const & id, string const & val, string const & curr)
                    {
                      hotelId = id;
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST_EQUAL(hotelId, kHotelId, ());
    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "ISK", ());
  }
}

UNIT_TEST(GetHotelInfo)
{
//  string const kHotelId = "0";  // Internal hotel id for testing.
//  booking::Api api;
//  booking::HotelInfo info;

//  api.GetHotelInfo(kHotelId, "en", [&info](booking::HotelInfo const & i)
//  {
//    info = i;
//    testing::StopEventLoop();
//  });
//  testing::RunEventLoop();

//  TEST_EQUAL(info.m_hotelId, kHotelId, ());
//  TEST(!info.m_description.empty(), ());
//  TEST_EQUAL(info.m_photos.size(), 2, ());
//  TEST_EQUAL(info.m_facilities.size(), 7, ());
//  TEST_EQUAL(info.m_reviews.size(), 4, ());
}
}
