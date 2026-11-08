#include "testing/testing.hpp"

#include "indexer/road_shields_parser.hpp"

UNIT_TEST(RoadShields_Smoke)
{
  using namespace ftypes;

  auto shields = GetRoadShields("France", "D 116A");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Orange, ());

  shields = GetRoadShields("Belarus", "M1");  // latin letter M
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red, ());

  shields = GetRoadShields("Belarus", "Е2");  // cyrillic letter Е
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Ukraine", "Р50");  // cyrillic letter Р
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Malaysia", "AH7");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Germany", "A 3;A 7");
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Germany", "blue/A 31;national/B 2R");
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Generic_Orange, ());

  shields = GetRoadShields("Germany", "TMC 33388 (St 2047)");
  TEST_EQUAL(shields.size(), 0, ());

  shields = GetRoadShields("US", "US:IN");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Default, ());

  shields = GetRoadShields("US", "SR 38;US:IN");
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_White, ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Default, ());

  shields = GetRoadShields("Switzerland", "e-road/E 67");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Estonia", "ee:national/27;ee:local/7841171");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Orange, ());

  shields = GetRoadShields("NZ", "SH 26");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red, ());
  TEST_EQUAL(shields[0].m_name, "26", ());

  shields = GetRoadShields("New Zealand North_Auckland", "NZ:SH/26");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red, ());
  TEST_EQUAL(shields[0].m_name, "26", ());
}

// https://github.com/organicmaps/organicmaps/issues/5934
// A road with ref "CTH NN" in a route relation with ref "NN" should get one shield, not two.
UNIT_TEST(RoadShields_RelationNumberOfWayRef)
{
  using namespace ftypes;

  auto shields = GetRoadShields("US_Wisconsin_Milwaukee", "CTH NN;US:WI:Milwaukee/NN");
  TEST_EQUAL(shields.size(), 1, (shields));
  TEST_EQUAL(shields[0].m_name, "CTH NN", (shields));

  shields = GetRoadShields("US_Minnesota_Minneapolis", "CR 12;US:MN:Hennepin/12");
  TEST_EQUAL(shields.size(), 1, (shields));
  TEST_EQUAL(shields[0].m_name, "CR 12", (shields));

  // Different roads keep both shields.
  shields = GetRoadShields("US_Wisconsin_Milwaukee", "CTH NN;US:WI:Milwaukee/MM");
  TEST_EQUAL(shields.size(), 2, (shields));
  shields = GetRoadShields("US_Minnesota_Minneapolis", "CR 112;US:MN:Hennepin/12");
  TEST_EQUAL(shields.size(), 2, (shields));
  // A relation shield with a network style is kept.
  shields = GetRoadShields("US_Wisconsin_Milwaukee", "WIS 12;US:WI/12");
  TEST_EQUAL(shields.size(), 2, (shields));
}
