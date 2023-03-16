#include "testing/testing.hpp"

#include "indexer/road_shields_parser.hpp"

UNIT_TEST(RoadShields_Smoke)
{
  using namespace ftypes;

  auto shields = GetRoadShields("France", "D 116A");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields.begin()->m_type, RoadShieldType::Generic_Orange, ());

  shields = GetRoadShields("Belarus", "M1"); // latin letter M
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields.begin()->m_type, RoadShieldType::Generic_Red, ());

  shields = GetRoadShields("Belarus", "Е2");  // cyrillic letter Е
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields.begin()->m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Ukraine", "Р50");  // cyrillic letter Р
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields.begin()->m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Malaysia", "AH7");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields.begin()->m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Germany", "A 3;A 7");
  TEST_EQUAL(shields.size(), 2, ());
  auto it = shields.begin();
  TEST_EQUAL(it->m_type, RoadShieldType::Generic_Blue, ());
  TEST_EQUAL((++it)->m_type, RoadShieldType::Generic_Blue, ());
}
