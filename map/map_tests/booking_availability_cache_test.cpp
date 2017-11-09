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
  Cache cache(2, 0);

  std::string kHotelId = "0";

  TEST_EQUAL(cache.Get(kHotelId), Cache::HotelStatus::Absent, ());

  cache.Reserve(kHotelId);

  TEST_EQUAL(cache.Get(kHotelId), Cache::HotelStatus::NotReady, ());

  cache.Insert(kHotelId, Cache::HotelStatus::Available);

  TEST_EQUAL(cache.Get(kHotelId), Cache::HotelStatus::Available, ());

  cache.Insert(kHotelId, Cache::HotelStatus::UnAvailable);

  TEST_EQUAL(cache.Get(kHotelId), Cache::HotelStatus::UnAvailable, ());
}

UNIT_TEST(AvailabilityCache_RemoveExtra)
{
  Cache cache(3, 0);
  std::vector<std::string> const kHotelIds = {"1", "2", "3"};

  for (auto const & id : kHotelIds)
    TEST_EQUAL(cache.Get(id), Cache::HotelStatus::Absent, ());

  for (auto const & id : kHotelIds)
    cache.Insert(id, Cache::HotelStatus::Available);

  for (auto const & id : kHotelIds)
    TEST_EQUAL(cache.Get(id), Cache::HotelStatus::Available, ());

  cache.Insert("4", Cache::HotelStatus::Available);

  for (auto const & id : kHotelIds)
    TEST_EQUAL(cache.Get(id), Cache::HotelStatus::Absent, ());

  TEST_EQUAL(cache.Get("4"), Cache::HotelStatus::Available, ());
}
}  // namespace