#include "testing/testing.hpp"

#include "partners_api/booking_api.hpp"

UNIT_TEST(Booking_SmokeTest)
{
  BookingApi api;

  string url = api.GetBookHotelUrl("http://someurl.com");
  TEST(!url.empty(), ());
}

UNIT_TEST(Booking_GetMinPrice)
{
  BookingApi api;
  api.SetTestingMode(true);
  string const kHotelId = "98251"; // Special hotel id for testing.

  {
    string price;
    string currency;
    api.GetMinPrice(kHotelId, BookingApi::kDefaultCurrency,
                    [&price, &currency](string const & val, string const & curr)
                    {
                      price = val;
                      currency = curr;
                      testing::StopEventLoop();
                    });
    testing::RunEventLoop();

    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "USD", ());
  }

  {
    string price;
    string currency;
    api.GetMinPrice(kHotelId, "RUB", [&price, &currency](string const & val, string const & curr)
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
    api.GetMinPrice(kHotelId, "ISK", [&price, &currency](string const & val, string const & curr)
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
