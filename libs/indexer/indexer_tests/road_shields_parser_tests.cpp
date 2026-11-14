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

// https://github.com/organicmaps/organicmaps/issues/1103
// The same road number written with and without a space should get one shield.
UNIT_TEST(RoadShields_SameNumberWithoutSpace)
{
  using namespace ftypes;

  auto shields = GetRoadShields("France_Provence", "D 106;FR:83/D106");
  TEST_EQUAL(shields.size(), 1, (shields));
  TEST_EQUAL(shields[0].m_name, "D 106", (shields));
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Orange, (shields));

  // A network style wins over the plain shield of the same number, in either order.
  shields = GetRoadShields("Poland_Masovian Voivodeship", "S 7;PL:national/S7");
  TEST_EQUAL(shields.size(), 1, (shields));
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red, (shields));
  shields = GetRoadShields("Poland_Masovian Voivodeship", "PL:national/S7;S 7");
  TEST_EQUAL(shields.size(), 1, (shields));
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red, (shields));

  // A modifier makes a different shield.
  shields = GetRoadShields("US", "US 30;US 30 Bus");
  TEST_EQUAL(shields.size(), 2, (shields));
  TEST_EQUAL(shields[1].m_additionalText, "Bus", (shields));

  // Different numbers or styles are kept.
  shields = GetRoadShields("France_Provence", "D 106;D 1060");
  TEST_EQUAL(shields.size(), 2, (shields));
  shields = GetRoadShields("France_Provence", "D 7;e-road/D7");
  TEST_EQUAL(shields.size(), 2, (shields));
}

// Search indexes every spelling, so that both "D 106" and "D106" find the road. The place page shows one.
UNIT_TEST(RoadShields_NamesSpellings)
{
  using Names = std::vector<std::string>;
  TEST_EQUAL(ftypes::GetRoadShieldsNames("D 106;FR:83/D106", true), Names({"D 106", "D106"}), ());
  TEST_EQUAL(ftypes::GetRoadShieldsNames("D106;FR:83/D 106", true), Names({"D106", "D 106"}), ());
  TEST_EQUAL(ftypes::GetRoadShieldsNames("M-11;ru:national/M-11", true), Names({"M-11"}), ());

  TEST_EQUAL(ftypes::GetRoadShieldsNames("D 106;FR:83/D106"), Names({"D 106"}), ());
  TEST_EQUAL(ftypes::GetRoadShieldsNames("D106;FR:83/D 106"), Names({"D106"}), ());
}
