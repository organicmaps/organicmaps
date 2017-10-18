#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/async_gui_thread.hpp"

#include "partners_api/booking_api.hpp"

#include "base/scope_guard.hpp"

#include <chrono>

using namespace partners_api;

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

UNIT_TEST(Booking_HotelAvailability)
{
  booking::AvailabilityParams params;
  params.m_hotelIds = {"98251"};
  params.m_rooms = {"A,A"};
  params.m_checkin = std::chrono::system_clock::now() + std::chrono::hours(24);
  params.m_checkout = std::chrono::system_clock::now() + std::chrono::hours(24 * 7);
  params.m_stars = {"4", "5"};
  string result;
  TEST(booking::RawApi::HotelAvailability(params, result), ());
  TEST(!result.empty(), ());
  LOG(LINFO, (result));
}

UNIT_CLASS_TEST(AsyncGuiThread, Booking_GetMinPrice)
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
                      testing::Notify();
                    });
    testing::Wait();

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
                      testing::Notify();
                    });
    testing::Wait();

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
                      testing::Notify();
                    });
    testing::Wait();

    TEST_EQUAL(hotelId, kHotelId, ());
    TEST(!price.empty(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "ISK", ());
  }
}

UNIT_CLASS_TEST(AsyncGuiThread, GetHotelInfo)
{
  string const kHotelId = "0";  // Internal hotel id for testing.
  booking::Api api;
  booking::HotelInfo info;

  api.GetHotelInfo(kHotelId, "en", [&info](booking::HotelInfo const & i)
  {
    info = i;
    testing::Notify();
  });
  testing::Wait();

  TEST_EQUAL(info.m_hotelId, kHotelId, ());
  TEST(!info.m_description.empty(), ());
  TEST_EQUAL(info.m_photos.size(), 2, ());
  TEST_EQUAL(info.m_facilities.size(), 7, ());
  TEST_EQUAL(info.m_reviews.size(), 4, ());
}

UNIT_CLASS_TEST(AsyncGuiThread, GetHotelAvailability)
{
  booking::SetBookingUrlForTesting("http://localhost:34568/booking/min_price");
  MY_SCOPE_GUARD(cleanup, []() { booking::SetBookingUrlForTesting(""); });

  booking::AvailabilityParams params;
  params.m_hotelIds = {"77615", "10623"};
  params.m_rooms = {"A,A"};
  params.m_checkin = std::chrono::system_clock::now() + std::chrono::hours(24);
  params.m_checkout = std::chrono::system_clock::now() + std::chrono::hours(24 * 7);
  params.m_stars = {"4"};
  booking::Api api;
  std::vector<uint64_t> result;

  api.GetHotelAvailability(params, [&result](std::vector<uint64_t> const & r)
  {
    result = r;
    testing::Notify();
  });
  testing::Wait();

  TEST_EQUAL(result.size(), 3, ());
  TEST_EQUAL(result[0], 10623, ());
  TEST_EQUAL(result[1], 10624, ());
  TEST_EQUAL(result[2], 10625, ());
}
}
