#include "testing/testing.hpp"

#include "map/booking_api.hpp"

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
