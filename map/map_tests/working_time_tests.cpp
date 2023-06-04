#include "testing/testing.hpp"

#include "map/osm_opening_hours.hpp"

using namespace osm;

UNIT_TEST(WorkingTime_OpenSoon)
{
  // 1-jan-2000 08:50
  std::tm when = {};
  when.tm_mday = 1;
  when.tm_mon = 0;
  when.tm_year = 100;
  when.tm_hour = 8;
  when.tm_min = 50;

  time_t now = std::mktime(&when);

  TEST_EQUAL(PlaceStateCheck("09:00-21:00", now), EPlaceState::OpenSoon, ());
}

UNIT_TEST(WorkingTime_CloseSoon)
{
  // 1-jan-2000 20:50
  std::tm when = {};
  when.tm_mday = 1;
  when.tm_mon = 0;
  when.tm_year = 100;
  when.tm_hour = 20;
  when.tm_min = 50;

  time_t now = std::mktime(&when);
  TEST_EQUAL(PlaceStateCheck("09:00-21:00", now), EPlaceState::CloseSoon, ());
}

UNIT_TEST(WorkingTime_Open)
{
  // 1-jan-2000 13:50
  std::tm when = {};
  when.tm_mday = 1;
  when.tm_mon = 0;
  when.tm_year = 100;
  when.tm_hour = 13;
  when.tm_min = 50;

  time_t now = std::mktime(&when);
  TEST_EQUAL(PlaceStateCheck("09:00-21:00", now), EPlaceState::Open, ());
}

UNIT_TEST(WorkingTime_Closed)
{
  // 1-jan-2000 06:50
  std::tm when = {};
  when.tm_mday = 1;
  when.tm_mon = 0;
  when.tm_year = 100;
  when.tm_hour = 6;
  when.tm_min = 50;

  time_t now = std::mktime(&when);
  TEST_EQUAL(PlaceStateCheck("09:00-21:00", now), EPlaceState::Closed, ());
}
