#include "testing/testing.hpp"

#include "partners_api/booking_api.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include <chrono>
#include <utility>

using namespace platform::tests_support;
using namespace booking;

namespace
{
class AsyncGuiThreadBooking : public AsyncGuiThread
{
public:
  AsyncGuiThreadBooking()
  {
    SetBookingUrlForTesting("http://localhost:34568/booking");
  }

  ~AsyncGuiThreadBooking() override
  {
    SetBookingUrlForTesting("");
  }
};

UNIT_TEST(Booking_GetHotelAvailability)
{
  string const kHotelId = "98251";  // Booking hotel id for testing.
  string result;
  TEST(RawApi::GetHotelAvailability(kHotelId, "", result), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Booking_GetExtendedInfo)
{
  string const kHotelId = "0";  // Internal hotel id for testing.
  string result;
  TEST(RawApi::GetExtendedInfo(kHotelId, "en", result), ());
  TEST(!result.empty(), ());
}

UNIT_TEST(Booking_HotelAvailability)
{
  AvailabilityParams params;
  params.m_hotelIds = {"98251"};
  params.m_rooms = {{2, AvailabilityParams::Room::kNoChildren}};
  params.m_checkin = std::chrono::system_clock::now() + std::chrono::hours(24);
  params.m_checkout = std::chrono::system_clock::now() + std::chrono::hours(24 * 7);
  params.m_stars = {"4", "5"};
  string result;
  TEST(RawApi::HotelAvailability(params, result), ());
  TEST(!result.empty(), ());
  LOG(LINFO, (result));
}

UNIT_CLASS_TEST(AsyncGuiThreadBooking, Booking_GetBlockAvailability)
{
  Api api;
  {
    auto params = BlockParams::MakeDefault();
    params.m_hotelId = "0";  // Internal hotel id for testing.
    auto price = BlockInfo::kIncorrectPrice;
    string currency;
    string hotelId;
    auto copy = params;
    api.GetBlockAvailability(std::move(copy), [&hotelId, &price, &currency](std::string const & id,
                                                                              Blocks const & blocks)
    {
      hotelId = id;
      price = blocks.m_totalMinPrice;
      currency = blocks.m_currency;
      testing::Notify();
    });
    testing::Wait();

    TEST_EQUAL(hotelId, params.m_hotelId, ());
    TEST_NOT_EQUAL(price, std::numeric_limits<double>::max(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "EUR", ());
  }

  {
    auto params = BlockParams::MakeDefault();
    params.m_hotelId = "0";  // Internal hotel id for testing.
    params.m_currency = "RUB";
    double price = std::numeric_limits<double>::max();
    string currency;
    string hotelId;
    auto copy = params;
    api.GetBlockAvailability(std::move(copy), [&hotelId, &price, &currency](std::string const & id,
                                                                              Blocks const & blocks)
    {
      hotelId = id;
      price = blocks.m_totalMinPrice;
      currency = blocks.m_currency;
      testing::Notify();
    });
    testing::Wait();

    TEST_EQUAL(hotelId, params.m_hotelId, ());
    TEST_NOT_EQUAL(price, std::numeric_limits<double>::max(), ());
    TEST(!currency.empty(), ());
    TEST_EQUAL(currency, "RUB", ());
  }
}

UNIT_CLASS_TEST(AsyncGuiThread, GetHotelInfo)
{
  string const kHotelId = "0";  // Internal hotel id for testing.
  Api api;
  HotelInfo info;

  api.GetHotelInfo(kHotelId, "en", [&info](HotelInfo const & i)
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

UNIT_CLASS_TEST(AsyncGuiThreadBooking, GetHotelAvailability)
{
  AvailabilityParams params;
  params.m_hotelIds = {"0"}; // Internal hotel id for testing.
  params.m_rooms = {{2, AvailabilityParams::Room::kNoChildren}};
  params.m_checkin = std::chrono::system_clock::now() + std::chrono::hours(24);
  params.m_checkout = std::chrono::system_clock::now() + std::chrono::hours(24 * 7);
  params.m_stars = {"4"};
  Api api;
  std::vector<std::string> result;

  api.GetHotelAvailability(params, [&result](std::vector<std::string> const & r)
  {
    result = r;
    testing::Notify();
  });
  testing::Wait();

  TEST_EQUAL(result.size(), 3, ());
  TEST_EQUAL(result[0], "10623", ());
  TEST_EQUAL(result[1], "10624", ());
  TEST_EQUAL(result[2], "10625", ());
}
}
