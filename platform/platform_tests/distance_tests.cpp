#include "testing/testing.hpp"

#include "platform/distance.hpp"

namespace platform
{
UNIT_TEST(Distance_IsValid)
{
  TEST_EQUAL(Distance(0.0).IsValid(), false, ());
  TEST_EQUAL(Distance(-1).IsValid(), false, ());
  TEST_EQUAL(Distance(std::numeric_limits<double>::epsilon()).IsValid(), false, ());
  TEST_EQUAL(Distance(1).IsValid(), true, ());
  TEST_EQUAL(Distance(1243.2, Distance::Units::Feet).IsValid(), true, ());
}

UNIT_TEST(Distance_Conversions)
{
  // TODO
}

UNIT_TEST(Distance_GetUnits)
{
  TEST_EQUAL(Distance(1234).GetUnits(), Distance::Units::Meters, ());
  TEST_EQUAL(Distance(1234, Distance::Units::Kilometers).GetUnits(), Distance::Units::Kilometers,
             ());
  TEST_EQUAL(Distance(1234, Distance::Units::Feet).GetUnits(), Distance::Units::Feet, ());
  TEST_EQUAL(Distance(1234, Distance::Units::Miles).GetUnits(), Distance::Units::Miles, ());
}

UNIT_TEST(Distance_GetUnitsString)
{
  TEST_EQUAL(Distance(1234).GetUnitsString(), "m", ());
  TEST_EQUAL(Distance(1234, Distance::Units::Meters).GetUnitsString(), "m", ());
  TEST_EQUAL(Distance(1234, Distance::Units::Kilometers).GetUnitsString(), "km", ());
  TEST_EQUAL(Distance(1234, Distance::Units::Feet).GetUnitsString(), "ft", ());
  TEST_EQUAL(Distance(1234, Distance::Units::Miles).GetUnitsString(), "mi", ());
}

UNIT_TEST(Distance_FormattedDistance)
{
  struct TestData
  {
    Distance distance;
    double formattedDistance;
    Distance::Units formattedUnits;
    std::string formattedDistanceString;
    std::string formattedString;
  };
  // clang-format off
  std::vector<TestData> testData{
    // From Meters to Meters
    {Distance(0,       Distance::Units::Meters),     0,    Distance::Units::Meters,     "0",    "0 m"},
    {Distance(1,       Distance::Units::Meters),     1,    Distance::Units::Meters,     "1",    "1 m"},
    {Distance(1.234,   Distance::Units::Meters),     1.2,  Distance::Units::Meters,     "1.2",  "1.2 m"},
    {Distance(9.99,    Distance::Units::Meters),     10,   Distance::Units::Meters,     "10",   "10 m"},
    {Distance(10.01,   Distance::Units::Meters),     10,   Distance::Units::Meters,     "10",   "10 m"},
    {Distance(64.2,    Distance::Units::Meters),     64,   Distance::Units::Meters,     "64",   "64 m"},

    // From Kilometers to Kilometers
    {Distance(0,       Distance::Units::Kilometers), 0,    Distance::Units::Kilometers, "0",    "0 km"},
    {Distance(1.234,   Distance::Units::Kilometers), 1.2,  Distance::Units::Kilometers, "1.2",  "1.2 km"},
    {Distance(10,      Distance::Units::Kilometers), 10,   Distance::Units::Kilometers, "10",   "10 km"},
    {Distance(99.99,   Distance::Units::Kilometers), 100,  Distance::Units::Kilometers, "100",  "100 km"},
    {Distance(100.01,  Distance::Units::Kilometers), 100,  Distance::Units::Kilometers, "100",  "100 km"},
    {Distance(1000,    Distance::Units::Kilometers), 1000, Distance::Units::Kilometers, "1000", "1000 km"},
    {Distance(1234,    Distance::Units::Kilometers), 1230, Distance::Units::Kilometers, "1230", "1230 km"},

    // From Feet to Feet
    {Distance(0,       Distance::Units::Feet),       0,    Distance::Units::Feet,       "0",    "0 ft"},
    {Distance(1,       Distance::Units::Feet),       1,    Distance::Units::Feet,       "1",    "1 ft"},
    {Distance(9.99,    Distance::Units::Feet),       10,   Distance::Units::Feet,       "10",   "10 ft"},
    {Distance(10.01,   Distance::Units::Feet),       10,   Distance::Units::Feet,       "10",   "10 ft"},

    // From Miles to Miles
    {Distance(0,       Distance::Units::Miles),      0,    Distance::Units::Miles,      "0",    "0 mi"},
    {Distance(1,       Distance::Units::Miles),      1,    Distance::Units::Miles,      "1",    "1 mi"},
    {Distance(1.234,   Distance::Units::Miles),      1.2,  Distance::Units::Miles,      "1.2",  "1.2 mi"},
    {Distance(9.99,    Distance::Units::Miles),      10,   Distance::Units::Miles,      "10",   "10 mi"},
    {Distance(10.01,   Distance::Units::Miles),      10,   Distance::Units::Miles,      "10",   "10 mi"},
    {Distance(998,     Distance::Units::Miles),      1000, Distance::Units::Miles,      "1000", "1000 mi"},
    {Distance(999,     Distance::Units::Miles),      1000, Distance::Units::Miles,      "1000", "1000 mi"},

    // From Meters to Kilometers
    {Distance(999,     Distance::Units::Meters),     1,    Distance::Units::Kilometers, "1",    "1 km"},
    {Distance(1000,    Distance::Units::Meters),     1,    Distance::Units::Kilometers, "1",    "1 km"},
    {Distance(1100,    Distance::Units::Meters),     1.1,  Distance::Units::Kilometers, "1.1",  "1.1 km"},
    {Distance(1500,    Distance::Units::Meters),     1.5,  Distance::Units::Kilometers, "1.5",  "1.5 km"},
    {Distance(287'386, Distance::Units::Meters),     290,  Distance::Units::Kilometers, "290",  "290 km"},

    // From Feet to Miles
    {Distance(999,     Distance::Units::Feet),       0.2,  Distance::Units::Miles,      "0.2",  "0.2 mi"},
    {Distance(1000,    Distance::Units::Feet),       0.2,  Distance::Units::Miles,      "0.2",  "0.2 mi"},
    {Distance(1150,    Distance::Units::Feet),       0.2,  Distance::Units::Miles,      "0.2",  "0.2 mi"},
    {Distance(5280,    Distance::Units::Feet),       1,    Distance::Units::Miles,      "1",    "1 mi"},
    {Distance(7920,    Distance::Units::Feet),       1.5,  Distance::Units::Miles,      "1.5",  "1.5 mi"},
    {Distance(10560,   Distance::Units::Feet),       2,    Distance::Units::Miles,      "2",    "2 mi"},
    {Distance(100'000, Distance::Units::Feet),       19,   Distance::Units::Miles,      "19",   "19 mi"},

    // Rounding checks: 112 -> 110, 998 -> 1000
    {Distance(115,     Distance::Units::Kilometers), 120,  Distance::Units::Kilometers, "120",  "120 km"},
    {Distance(130,     Distance::Units::Kilometers), 130,  Distance::Units::Kilometers, "130",  "130 km"},
    {Distance(980,     Distance::Units::Kilometers), 980,  Distance::Units::Kilometers, "980",  "980 km"},
    {Distance(1050,    Distance::Units::Kilometers), 1050, Distance::Units::Kilometers, "1050", "1050 km"},

    {Distance(105,     Distance::Units::Miles),      110,  Distance::Units::Miles,      "110",  "110 mi"},
    {Distance(145,     Distance::Units::Miles),      150,  Distance::Units::Miles,      "150",  "150 mi"},
    {Distance(1150,    Distance::Units::Miles),      1150, Distance::Units::Miles,      "1150", "1150 mi"},

    {Distance(95,      Distance::Units::Feet),       95,   Distance::Units::Feet,       "95",   "95 ft"},
    {Distance(125,     Distance::Units::Feet),       130,  Distance::Units::Feet,       "130",  "130 ft"},
    {Distance(991,     Distance::Units::Feet),       990,  Distance::Units::Feet,       "990",  "990 ft"},
  };
  // clang-format on
  for (TestData const & data : testData)
  {
    Distance const formattedDistance = data.distance.GetFormattedDistance();
    // Run two times to verify that nothing breaks after second format
    for (const auto & d : {formattedDistance, formattedDistance.GetFormattedDistance()})
    {
      TEST_EQUAL(d.GetDistance(), data.formattedDistance, ());
      TEST_EQUAL(d.GetUnits(), data.formattedUnits, ());
      TEST_EQUAL(d.GetDistanceString(), data.formattedDistanceString, ());
      TEST_EQUAL(d.ToString(), data.formattedString, ());
    }
  }
}
}  // namespace platform
