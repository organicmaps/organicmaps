#include "testing/testing.hpp"

#include "platform/distance.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

namespace platform
{
struct ScopedSettings
{
  /// Saves/restores previous units and sets new units for a scope.
  explicit ScopedSettings(measurement_utils::Units newUnits) : m_oldUnits(measurement_utils::Units::Metric)
  {
    m_wasSet = settings::Get(settings::kMeasurementUnits, m_oldUnits);
    settings::Set(settings::kMeasurementUnits, newUnits);
  }

  ~ScopedSettings()
  {
    if (m_wasSet)
      settings::Set(settings::kMeasurementUnits, m_oldUnits);
    else
      settings::Delete(settings::kMeasurementUnits);
  }

  bool m_wasSet;
  measurement_utils::Units m_oldUnits;
};

UNIT_TEST(Distance_InititalDistance)
{
  Distance d;
  TEST(!d.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), -1.0, ());
  TEST_EQUAL(d.GetDistanceString(), "", ());
  TEST_EQUAL(d.ToString(), "", ());
}

UNIT_TEST(Distance_CreateFormatted)
{
  {
    ScopedSettings guard(measurement_utils::Units::Metric);

    Distance d = Distance::CreateFormatted(100);
    TEST_EQUAL(d.GetUnits(), Distance::Units::Meters, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 100.0, ());
    TEST_EQUAL(d.GetDistanceString(), "100", ());
    TEST_EQUAL(d.ToString(), "100 m", ());
  }
  {
    ScopedSettings guard(measurement_utils::Units::Imperial);

    Distance d = Distance::CreateFormatted(100);
    TEST_EQUAL(d.GetUnits(), Distance::Units::Feet, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 330.0, ());
    TEST_EQUAL(d.GetDistanceString(), "330", ());
    TEST_EQUAL(d.ToString(), "330 ft", ());
  }
}

UNIT_TEST(Distance_CreateAltitudeFormatted)
{
  {
    ScopedSettings guard(measurement_utils::Units::Metric);

    Distance d = Distance::CreateAltitudeFormatted(5);
    TEST_EQUAL(d.GetUnits(), Distance::Units::Meters, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 5.0, ());
    TEST_EQUAL(d.GetDistanceString(), "5", ());
    TEST_EQUAL(d.ToString(), "5 m", ());
  }
  {
    ScopedSettings guard(measurement_utils::Units::Imperial);

    Distance d = Distance::CreateAltitudeFormatted(10000);
    TEST_EQUAL(d.GetUnits(), Distance::Units::Feet, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 32808.0, ());
    TEST_EQUAL(d.GetDistanceString(), "32808", ());
    TEST_EQUAL(d.ToString(), "32808 ft", ());
  }
}

UNIT_TEST(Distance_IsLowUnits)
{
  TEST_EQUAL(Distance(0.0, Distance::Units::Meters).IsLowUnits(), true, ());
  TEST_EQUAL(Distance(0.0, Distance::Units::Feet).IsLowUnits(), true, ());
  TEST_EQUAL(Distance(0.0, Distance::Units::Kilometers).IsLowUnits(), false, ());
  TEST_EQUAL(Distance(0.0, Distance::Units::Miles).IsLowUnits(), false, ());
}

UNIT_TEST(Distance_IsHighUnits)
{
  TEST_EQUAL(Distance(0.0, Distance::Units::Meters).IsHighUnits(), false, ());
  TEST_EQUAL(Distance(0.0, Distance::Units::Feet).IsHighUnits(), false, ());
  TEST_EQUAL(Distance(0.0, Distance::Units::Kilometers).IsHighUnits(), true, ());
  TEST_EQUAL(Distance(0.0, Distance::Units::Miles).IsHighUnits(), true, ());
}

UNIT_TEST(Distance_To)
{
  using Units = Distance::Units;

  struct TestData
  {
    double initialDistance;
    Distance::Units initialUnits;
    Distance::Units to;
    double newDistance;
    Distance::Units newUnits;
  };
  // clang-format off
  std::vector<TestData> testData{
    {0.1,       Units::Meters,     Units::Feet,       0,    Units::Feet},
    {0.3,       Units::Meters,     Units::Feet,       1,    Units::Feet},
    {0.3048,    Units::Meters,     Units::Feet,       1,    Units::Feet},
    {0.4573,    Units::Meters,     Units::Feet,       2,    Units::Feet},
    {0.9,       Units::Meters,     Units::Feet,       3,    Units::Feet},
    {3,         Units::Meters,     Units::Feet,       10,   Units::Feet},
    {30.17,     Units::Meters,     Units::Feet,       99,   Units::Feet},
    {30.33,     Units::Meters,     Units::Feet,       100,  Units::Feet},
    {30.49,     Units::Meters,     Units::Feet,       100,  Units::Feet},
    {33.5,      Units::Meters,     Units::Feet,       110,  Units::Feet},
    {302,       Units::Meters,     Units::Feet,       990,  Units::Feet},
    {304.7,     Units::Meters,     Units::Feet,       0.2,  Units::Miles},
    {304.8,     Units::Meters,     Units::Feet,       0.2,  Units::Miles},
    {402.3,     Units::Meters,     Units::Feet,       0.2,  Units::Miles},
    {402.4,     Units::Meters,     Units::Feet,       0.3,  Units::Miles},
    {482.8,     Units::Meters,     Units::Feet,       0.3,  Units::Miles},
    {1609.3,    Units::Meters,     Units::Feet,       1,    Units::Miles},
    {1610,      Units::Meters,     Units::Feet,       1,    Units::Miles},
    {1770,      Units::Meters,     Units::Feet,       1.1,  Units::Miles},
    {15933,     Units::Meters,     Units::Feet,       9.9,  Units::Miles},
    {16093,     Units::Meters,     Units::Feet,       10,   Units::Miles},
    {16093.5,   Units::Meters,     Units::Feet,       10,   Units::Miles},
    {16898.113, Units::Meters,     Units::Feet,       11,   Units::Miles},
    {16898.113, Units::Meters,     Units::Kilometers, 17,   Units::Kilometers},
    {302,       Units::Meters,     Units::Miles,      0.2,  Units::Miles},
    {0.1,       Units::Kilometers, Units::Meters,     100,  Units::Meters},
    {12,        Units::Kilometers, Units::Feet,       7.5,  Units::Miles},
    {0.1,       Units::Kilometers, Units::Feet,       330,  Units::Feet},
    {110,       Units::Feet,       Units::Meters,     34,   Units::Meters},
    {1100,      Units::Feet,       Units::Kilometers, 0.3,  Units::Kilometers},
    {1100,      Units::Feet,       Units::Meters,     340,  Units::Meters},
    {1100,      Units::Feet,       Units::Miles,      0.2,  Units::Miles},
    {0.2,       Units::Miles,      Units::Meters,     320,  Units::Meters},
    {11,        Units::Miles,      Units::Meters,     18,   Units::Kilometers},
    {11,        Units::Miles,      Units::Kilometers, 18,   Units::Kilometers},
    {0.1,       Units::Miles,      Units::Feet,       530,  Units::Feet},
  };
  // clang-format on
  for (TestData const & data : testData)
  {
    Distance const formattedDistance =
        Distance(data.initialDistance, data.initialUnits).To(data.to).GetFormattedDistance();
    TEST_ALMOST_EQUAL_ULPS(formattedDistance.GetDistance(), data.newDistance, (formattedDistance.ToString()));
    TEST_EQUAL(formattedDistance.GetUnits(), data.newUnits, (formattedDistance.ToString()));
  }
}

UNIT_TEST(Distance_ToPlatformUnitsFormatted)
{
  {
    ScopedSettings guard(measurement_utils::Units::Metric);

    Distance d{11, Distance::Units::Feet};
    Distance newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Meters, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 3.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "3", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), "3 m", (d.ToString()));

    d = Distance{11, Distance::Units::Kilometers};
    newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Kilometers, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 11.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "11", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), "11 km", (d.ToString()));
  }

  {
    ScopedSettings guard(measurement_utils::Units::Imperial);

    Distance d{11, Distance::Units::Feet};
    Distance newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Feet, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 11.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "11", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), "11 ft", (d.ToString()));

    d = Distance{11, Distance::Units::Kilometers};
    newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Miles, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 6.8, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "6.8", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), "6.8 mi", (d.ToString()));
  }
}

UNIT_TEST(Distance_GetUnits)
{
  TEST_EQUAL(Distance(1234).GetUnits(), Distance::Units::Meters, ());
  TEST_EQUAL(Distance(1234, Distance::Units::Kilometers).GetUnits(), Distance::Units::Kilometers, ());
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
  using Units = Distance::Units;

  struct TestData
  {
    Distance distance;
    double formattedDistance;
    Units formattedUnits;
    std::string formattedDistanceString;
    std::string formattedString;
  };
  // clang-format off
  std::vector<TestData> testData{
    // From Meters to Meters
    {Distance(0,       Units::Meters),     0,    Units::Meters,     "0",    "0 m"},
    {Distance(0.3,     Units::Meters),     0,    Units::Meters,     "0",    "0 m"},
    {Distance(0.9,     Units::Meters),     1,    Units::Meters,     "1",    "1 m"},
    {Distance(1,       Units::Meters),     1,    Units::Meters,     "1",    "1 m"},
    {Distance(1.234,   Units::Meters),     1,    Units::Meters,     "1",    "1 m"},
    {Distance(9.99,    Units::Meters),     10,   Units::Meters,     "10",   "10 m"},
    {Distance(10.01,   Units::Meters),     10,   Units::Meters,     "10",   "10 m"},
    {Distance(10.4,    Units::Meters),     10,   Units::Meters,     "10",   "10 m"},
    {Distance(10.5,    Units::Meters),     11,   Units::Meters,     "11",   "11 m"},
    {Distance(10.51,   Units::Meters),     11,   Units::Meters,     "11",   "11 m"},
    {Distance(64.2,    Units::Meters),     64,   Units::Meters,     "64",   "64 m"},
    {Distance(99,      Units::Meters),     99,   Units::Meters,     "99",   "99 m"},
    {Distance(100,     Units::Meters),     100,  Units::Meters,     "100",  "100 m"},
    {Distance(101,     Units::Meters),     100,  Units::Meters,     "100",  "100 m"},
    {Distance(109,     Units::Meters),     110,  Units::Meters,     "110",  "110 m"},
    {Distance(991,     Units::Meters),     990,  Units::Meters,     "990",  "990 m"},

    // From Kilometers to Kilometers
    {Distance(0,       Units::Kilometers), 0,    Units::Kilometers, "0",    "0 km"},
    {Distance(1.234,   Units::Kilometers), 1.2,  Units::Kilometers, "1.2",  "1.2 km"},
    {Distance(10,      Units::Kilometers), 10,   Units::Kilometers, "10",   "10 km"},
    {Distance(11,      Units::Kilometers), 11,   Units::Kilometers, "11",   "11 km"},
    {Distance(54,      Units::Kilometers), 54,   Units::Kilometers, "54",   "54 km"},
    {Distance(99.99,   Units::Kilometers), 100,  Units::Kilometers, "100",  "100 km"},
    {Distance(100.01,  Units::Kilometers), 100,  Units::Kilometers, "100",  "100 km"},
    {Distance(115,     Units::Kilometers), 115,  Units::Kilometers, "115",  "115 km"},
    {Distance(130,     Units::Kilometers), 130,  Units::Kilometers, "130",  "130 km"},
    {Distance(980,     Units::Kilometers), 980,  Units::Kilometers, "980",  "980 km"},
    {Distance(999,     Units::Kilometers), 999,  Units::Kilometers, "999",  "999 km"},
    {Distance(1000,    Units::Kilometers), 1000, Units::Kilometers, "1000", "1000 km"},
    {Distance(1049.99, Units::Kilometers), 1050, Units::Kilometers, "1050", "1050 km"},
    {Distance(1050,    Units::Kilometers), 1050, Units::Kilometers, "1050", "1050 km"},
    {Distance(1050.01, Units::Kilometers), 1050, Units::Kilometers, "1050", "1050 km"},
    {Distance(1234,    Units::Kilometers), 1234, Units::Kilometers, "1234", "1234 km"},

    // From Feet to Feet
    {Distance(0,       Units::Feet),       0,    Units::Feet,       "0",    "0 ft"},
    {Distance(1,       Units::Feet),       1,    Units::Feet,       "1",    "1 ft"},
    {Distance(9.99,    Units::Feet),       10,   Units::Feet,       "10",   "10 ft"},
    {Distance(10.01,   Units::Feet),       10,   Units::Feet,       "10",   "10 ft"},
    {Distance(95,      Units::Feet),       95,   Units::Feet,       "95",   "95 ft"},
    {Distance(125,     Units::Feet),       130,  Units::Feet,       "130",  "130 ft"},
    {Distance(991,     Units::Feet),       990,  Units::Feet,       "990",  "990 ft"},

    // From Miles to Miles
    {Distance(0,       Units::Miles),      0,    Units::Miles,      "0",    "0 mi"},
    {Distance(1,       Units::Miles),      1,    Units::Miles,      "1",    "1 mi"},
    {Distance(1.234,   Units::Miles),      1.2,  Units::Miles,      "1.2",  "1.2 mi"},
    {Distance(9.99,    Units::Miles),      10,   Units::Miles,      "10",   "10 mi"},
    {Distance(10.01,   Units::Miles),      10,   Units::Miles,      "10",   "10 mi"},
    {Distance(11,      Units::Miles),      11,   Units::Miles,      "11",   "11 mi"},
    {Distance(54,      Units::Miles),      54,   Units::Miles,      "54",   "54 mi"},
    {Distance(105,     Units::Miles),      105,  Units::Miles,      "105",  "105 mi"},
    {Distance(145,     Units::Miles),      145,  Units::Miles,      "145",  "145 mi"},
    {Distance(150,     Units::Miles),      150,  Units::Miles,      "150",  "150 mi"},
    {Distance(998,     Units::Miles),      998,  Units::Miles,      "998",  "998 mi"},
    {Distance(999,     Units::Miles),      999,  Units::Miles,      "999",  "999 mi"},
    {Distance(1149.99, Units::Miles),      1150, Units::Miles,      "1150", "1150 mi"},
    {Distance(1150,    Units::Miles),      1150, Units::Miles,      "1150", "1150 mi"},
    {Distance(1150.01, Units::Miles),      1150, Units::Miles,      "1150", "1150 mi"},

    // From Meters to Kilometers
    {Distance(999,     Units::Meters),     1,    Units::Kilometers, "1",    "1 km"},
    {Distance(1000,    Units::Meters),     1,    Units::Kilometers, "1",    "1 km"},
    {Distance(1001,    Units::Meters),     1,    Units::Kilometers, "1",    "1 km"},
    {Distance(1100,    Units::Meters),     1.1,  Units::Kilometers, "1.1",  "1.1 km"},
    {Distance(1140,    Units::Meters),     1.1,  Units::Kilometers, "1.1",  "1.1 km"},
    {Distance(1151,    Units::Meters),     1.2,  Units::Kilometers, "1.2",  "1.2 km"},
    {Distance(1500,    Units::Meters),     1.5,  Units::Kilometers, "1.5",  "1.5 km"},
    {Distance(1549.9,  Units::Meters),     1.5,  Units::Kilometers, "1.5",  "1.5 km"},
    {Distance(1550,    Units::Meters),     1.6,  Units::Kilometers, "1.6",  "1.6 km"},
    {Distance(1551,    Units::Meters),     1.6,  Units::Kilometers, "1.6",  "1.6 km"},
    {Distance(9949,    Units::Meters),     9.9,  Units::Kilometers, "9.9",  "9.9 km"},
    {Distance(9992,    Units::Meters),     10,   Units::Kilometers, "10",   "10 km"},
    {Distance(10000,   Units::Meters),     10,   Units::Kilometers, "10",   "10 km"},
    {Distance(10499.9, Units::Meters),     10,   Units::Kilometers, "10",   "10 km"},
    {Distance(10501,   Units::Meters),     11,   Units::Kilometers, "11",   "11 km"},
    {Distance(54000,   Units::Meters),     54,   Units::Kilometers, "54",   "54 km"},
    {Distance(101'001, Units::Meters),     101,  Units::Kilometers, "101",  "101 km"},
    {Distance(101'999, Units::Meters),     102,  Units::Kilometers, "102",  "102 km"},
    {Distance(287'386, Units::Meters),     287,  Units::Kilometers, "287",  "287 km"},

    // From Feet to Miles
    {Distance(999,     Units::Feet),       0.2,  Units::Miles,      "0.2",  "0.2 mi"},
    {Distance(1000,    Units::Feet),       0.2,  Units::Miles,      "0.2",  "0.2 mi"},
    {Distance(1150,    Units::Feet),       0.2,  Units::Miles,      "0.2",  "0.2 mi"},
    {Distance(5280,    Units::Feet),       1,    Units::Miles,      "1",    "1 mi"},
    {Distance(7920,    Units::Feet),       1.5,  Units::Miles,      "1.5",  "1.5 mi"},
    {Distance(10560,   Units::Feet),       2,    Units::Miles,      "2",    "2 mi"},
    {Distance(100'000, Units::Feet),       19,   Units::Miles,      "19",   "19 mi"},
    {Distance(285'120, Units::Feet),       54,   Units::Miles,      "54",   "54 mi"},
    {Distance(633'547, Units::Feet),       120,  Units::Miles,      "120",  "120 mi"},
    {Distance(633'600, Units::Feet),       120,  Units::Miles,      "120",  "120 mi"},
    {Distance(633'653, Units::Feet),       120,  Units::Miles,      "120",  "120 mi"},
    {Distance(999'999, Units::Feet),       189,  Units::Miles,      "189",  "189 mi"},
  };
  // clang-format on
  for (TestData const & data : testData)
  {
    Distance const formattedDistance = data.distance.GetFormattedDistance();
    // Run two times to verify that nothing breaks after second format
    for (auto const & d : {formattedDistance, formattedDistance.GetFormattedDistance()})
    {
      TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), data.formattedDistance, (data.distance.ToString()));
      TEST_EQUAL(d.GetUnits(), data.formattedUnits, (data.distance.ToString()));
      TEST_EQUAL(d.GetDistanceString(), data.formattedDistanceString, (data.distance.ToString()));
      TEST_EQUAL(d.ToString(), data.formattedString, (data.distance.ToString()));
    }
  }
}
}  // namespace platform
