#include "testing/testing.hpp"

#include "partners_api/booking_api.hpp"

namespace
{
string const kHotelId = "98251";  // Special hotel id for testing.

UNIT_TEST(Booking_GetHotelAvailability)
{
  string result;
  TEST(booking::RawApi::GetHotelAvailability(kHotelId, "", result, true), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Booking_GetExtendedInfo)
{
  string result;
  TEST(booking::RawApi::GetExtendedInfo(kHotelId, "", result), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Booking_GetMinPrice)
{
  booking::Api api;
  api.SetTestingMode(true);

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
  booking::Api api;
  booking::HotelInfo info;

  api.GetHotelInfo(kHotelId, "en", [&info](booking::HotelInfo const & i)
  {
    info = i;
  });

  TEST_EQUAL(info.m_hotelId, kHotelId, ());
  TEST(!info.m_description.empty(), ());
  TEST_EQUAL(info.m_photos.size(), 5, ());
  TEST_EQUAL(info.m_facilities.size(), 3, ());
  TEST_EQUAL(info.m_reviews.size(), 12, ());
}
}
