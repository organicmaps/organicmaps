#include "testing/testing.hpp"

#include "partners_api/booking_api.hpp"

UNIT_TEST(Booking_SmokeTest)
{
  BookingApi api;

  string url = api.GetBookingUrl("http://someurl.com");
  TEST(!url.empty(), ());
}

UNIT_TEST(Booking_GetMinPrice)
{
  BookingApi api;

  {
    string price;
    string currency;
    api.GetMinPrice("10340", BookingApi::kDefaultCurrency,
                    [&price, &currency](string const & val, string const & curr)
                    {
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "EUR", ());
  }

  {
    string price;
    string currency;
    api.GetMinPrice("10340", "RUB", [&price, &currency](string const & val, string const & curr)
                    {
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "RUB", ());
  }

  {
    string price;
    string currency;
    api.GetMinPrice("10340", "ISK", [&price, &currency](string const & val, string const & curr)
                    {
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "ISK", ());
  }
}

UNIT_TEST(GetHotelInfo)  // GetHotelInfo is a mockup now.
{
  BookingApi api;
  BookingApi::HotelInfo info;

  api.GetHotelInfo("000", "en", [&info](BookingApi::HotelInfo const & i)
  {
    info = i;
  });

  TEST(!info.m_description.empty(), ());
  TEST_EQUAL(info.m_photos.size(), 9, ());
  TEST_EQUAL(info.m_facilities.size(), 3, ());
  TEST_EQUAL(info.m_reviews.size(), 12, ());
}
