#include "testing/testing.hpp"

#include "map/booking_filter_cache.hpp"

#include <chrono>
#include <string>

using namespace booking::filter::availability;
using namespace std::chrono;

namespace
{
UNIT_TEST(AvailabilityCache_Smoke)
{
  Cache cache(2 /* maxCount */, 0 /* expiryPeriodSeconds */);

  std::string kHotelId = "0";

  auto info = cache.Get(kHotelId);
  TEST_EQUAL(info.m_status, Cache::HotelStatus::Absent, ());
  TEST(!info.m_extras, ());

  cache.InsertNotReady(kHotelId);

  info = cache.Get(kHotelId);
  TEST_EQUAL(info.m_status, Cache::HotelStatus::NotReady, ());
  TEST(!info.m_extras, ());

  cache.InsertAvailable(kHotelId, {10.0, "Y"});

  info = cache.Get(kHotelId);
  TEST_EQUAL(info.m_status, Cache::HotelStatus::Available, ());
  TEST(info.m_extras, ());
  TEST_EQUAL(info.m_extras->m_currency, "Y", ());

  cache.InsertUnavailable(kHotelId);

  info = cache.Get(kHotelId);
  TEST_EQUAL(info.m_status, Cache::HotelStatus::Unavailable, ());
  TEST(!info.m_extras, ());
}

UNIT_TEST(AvailabilityCache_RemoveExtra)
{
  Cache cache(3 /* maxCount */, 0 /* expiryPeriodSeconds */);
  std::vector<std::string> const kHotelIds = {"1", "2", "3"};

  for (auto const & id : kHotelIds)
    TEST_EQUAL(cache.Get(id).m_status, Cache::HotelStatus::Absent, ());

  for (auto const & id : kHotelIds)
    cache.InsertAvailable(id, {1.0, "X"});

  for (auto const & id : kHotelIds)
    TEST_EQUAL(cache.Get(id).m_status, Cache::HotelStatus::Available, ());

  cache.InsertAvailable("4", {1.0, "X"});

  for (auto const & id : kHotelIds)
    TEST_EQUAL(cache.Get(id).m_status, Cache::HotelStatus::Absent, ());

  TEST_EQUAL(cache.Get("4").m_status, Cache::HotelStatus::Available, ());
}
}  // namespace
