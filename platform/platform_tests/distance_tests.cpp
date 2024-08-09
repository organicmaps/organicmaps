#include "testing/testing.hpp"

#include "platform/distance.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

namespace platform
{
std::string MakeDistanceStr(std::string const & value, std::string const & unit)
{
  return value + kNarrowNonBreakingSpace + unit;
}

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
  Distance const d;
  TEST(!d.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), -1.0, ());
  TEST_EQUAL(d.GetDistanceString(), "", ());
  TEST_EQUAL(d.ToString(), "", ());
}

UNIT_TEST(Distance_CreateFormatted)
{
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    Distance const d = Distance::CreateFormatted(100);
    TEST_EQUAL(d.GetUnits(), Distance::Units::Meters, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 100.0, ());
    TEST_EQUAL(d.GetDistanceString(), "100", ());
    TEST_EQUAL(d.ToString(), MakeDistanceStr("100", "m"), ());
  }
  {
    ScopedSettings const guard(measurement_utils::Units::Imperial);

    Distance const d = Distance::CreateFormatted(100);
    TEST_EQUAL(d.GetUnits(), Distance::Units::Feet, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 330.0, ());
    TEST_EQUAL(d.GetDistanceString(), "330", ());
    TEST_EQUAL(d.ToString(), MakeDistanceStr("330", "ft"), ());
  }
}

UNIT_TEST(Distance_CreateAltitudeFormatted)
{
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    TEST_EQUAL(Distance::FormatAltitude(5), MakeDistanceStr("5", "m"), ());
  }
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    TEST_EQUAL(Distance::FormatAltitude(-8849), MakeDistanceStr("-8849", "m"), ());
  }
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    TEST_EQUAL(Distance::FormatAltitude(12345), MakeDistanceStr("12,345", "m"), ());
  }
  {
    ScopedSettings const guard(measurement_utils::Units::Imperial);

    TEST_EQUAL(Distance::FormatAltitude(10000), MakeDistanceStr("32,808", "ft"), ());
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
  struct TestData
  {
    double initialDistance;
    Distance::Units initialUnits;
    Distance::Units to;
    double newDistance;
    Distance::Units newUnits;
  };

  using enum Distance::Units;
  // clang-format off
  TestData constexpr testData[] = {
    {0.1,       Meters,     Feet,       0,    Feet},
    {0.3,       Meters,     Feet,       1,    Feet},
    {0.3048,    Meters,     Feet,       1,    Feet},
    {0.4573,    Meters,     Feet,       2,    Feet},
    {0.9,       Meters,     Feet,       3,    Feet},
    {3,         Meters,     Feet,       10,   Feet},
    {30.17,     Meters,     Feet,       99,   Feet},
    {30.33,     Meters,     Feet,       100,  Feet},
    {30.49,     Meters,     Feet,       100,  Feet},
    {33.5,      Meters,     Feet,       110,  Feet},
    {302,       Meters,     Feet,       990,  Feet},
    {304.7,     Meters,     Feet,       0.2,  Miles},
    {304.8,     Meters,     Feet,       0.2,  Miles},
    {402.3,     Meters,     Feet,       0.2,  Miles},
    {402.4,     Meters,     Feet,       0.3,  Miles},
    {482.8,     Meters,     Feet,       0.3,  Miles},
    {1609.3,    Meters,     Feet,       1.0,  Miles},
    {1610,      Meters,     Feet,       1.0,  Miles},
    {1770,      Meters,     Feet,       1.1,  Miles},
    {15933,     Meters,     Feet,       9.9,  Miles},
    {16093,     Meters,     Feet,       10,   Miles},
    {16093.5,   Meters,     Feet,       10,   Miles},
    {16898.464, Meters,     Feet,       11,   Miles},
    {16898.113, Meters,     Kilometers, 17,   Kilometers},
    {302,       Meters,     Miles,      990,  Feet},
    {994,       Meters,     Kilometers, 990,  Meters},
    {995,       Meters,     Kilometers, 1.0,  Kilometers},
    {0.1,       Kilometers, Meters,     100,  Meters},
    {0.3,       Kilometers, Kilometers, 300,  Meters},
    {12,        Kilometers, Feet,       7.5,  Miles},
    {0.1,       Kilometers, Feet,       330,  Feet},
    {110,       Feet,       Meters,     34,   Meters},
    {1100,      Feet,       Kilometers, 340,  Meters},
    {1100,      Feet,       Meters,     340,  Meters},
    {1100,      Feet,       Miles,      0.2,  Miles},
    {0.2,       Miles,      Meters,     320,  Meters},
    {11,        Miles,      Meters,     18,   Kilometers},
    {11,        Miles,      Kilometers, 18,   Kilometers},
    {0.1,       Miles,      Feet,       530,  Feet},
  };

  // clang-format on
  for (TestData const & data : testData)
  {
    Distance const formatted =
        Distance(data.initialDistance, data.initialUnits).To(data.to).GetFormattedDistance();
    TEST_ALMOST_EQUAL_ULPS(formatted.GetDistance(), data.newDistance, (data.initialDistance));
    TEST_EQUAL(formatted.GetUnits(), data.newUnits, ());
  }
}

UNIT_TEST(Distance_ToPlatformUnitsFormatted)
{
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    Distance d{11, Distance::Units::Feet};
    Distance newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Meters, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 3.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "3", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("3", "m"), (d.ToString()));

    d = Distance{11, Distance::Units::Kilometers};
    newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Kilometers, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 11.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "11", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("11", "km"), (d.ToString()));
  }

  {
    ScopedSettings const guard(measurement_utils::Units::Imperial);

    Distance d{11, Distance::Units::Feet};
    Distance newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Feet, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 11.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "11", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("11", "ft"), (d.ToString()));

    d = Distance{11, Distance::Units::Kilometers};
    newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Distance::Units::Miles, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 6.8, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "6.8", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("6.8", "mi"), (d.ToString()));
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
  struct TestData
  {
    Distance distance;
    double formattedDistance;
    Distance::Units formattedUnits;
    std::string formattedDistanceString;
    std::string formattedString;
  };

  using enum Distance::Units;
  // clang-format off
  TestData const testData[] = {
    // From Meters to Meters
    {Distance(0,       Meters),     0,     Meters,     "0",      MakeDistanceStr("0", "m")},
    {Distance(0.3,     Meters),     0,     Meters,     "0",      MakeDistanceStr("0", "m")},
    {Distance(0.9,     Meters),     1,     Meters,     "1",      MakeDistanceStr("1", "m")},
    {Distance(1,       Meters),     1,     Meters,     "1",      MakeDistanceStr("1", "m")},
    {Distance(1.234,   Meters),     1,     Meters,     "1",      MakeDistanceStr("1", "m")},
    {Distance(9.99,    Meters),     10,    Meters,     "10",     MakeDistanceStr("10", "m")},
    {Distance(10.01,   Meters),     10,    Meters,     "10",     MakeDistanceStr("10", "m")},
    {Distance(10.4,    Meters),     10,    Meters,     "10",     MakeDistanceStr("10", "m")},
    {Distance(10.5,    Meters),     11,    Meters,     "11",     MakeDistanceStr("11", "m")},
    {Distance(10.51,   Meters),     11,    Meters,     "11",     MakeDistanceStr("11", "m")},
    {Distance(64.2,    Meters),     64,    Meters,     "64",     MakeDistanceStr("64", "m")},
    {Distance(99,      Meters),     99,    Meters,     "99",     MakeDistanceStr("99", "m")},
    {Distance(100,     Meters),     100,   Meters,     "100",    MakeDistanceStr("100", "m")},
    {Distance(101,     Meters),     100,   Meters,     "100",    MakeDistanceStr("100", "m")},
    {Distance(109,     Meters),     110,   Meters,     "110",    MakeDistanceStr("110", "m")},
    {Distance(991,     Meters),     990,   Meters,     "990",    MakeDistanceStr("990", "m")},

    // From Kilometers to Kilometers
    {Distance(0,       Kilometers), 0,     Meters,     "0",      MakeDistanceStr("0", "m")},
    {Distance(0.3,     Kilometers), 300,   Meters,     "300",    MakeDistanceStr("300", "m")},
    {Distance(1.234,   Kilometers), 1.2,   Kilometers, "1.2",    MakeDistanceStr("1.2", "km")},
    {Distance(10,      Kilometers), 10,    Kilometers, "10",     MakeDistanceStr("10", "km")},
    {Distance(11,      Kilometers), 11,    Kilometers, "11",     MakeDistanceStr("11", "km")},
    {Distance(54,      Kilometers), 54,    Kilometers, "54",     MakeDistanceStr("54", "km")},
    {Distance(99.99,   Kilometers), 100,   Kilometers, "100",    MakeDistanceStr("100", "km")},
    {Distance(100.01,  Kilometers), 100,   Kilometers, "100",    MakeDistanceStr("100", "km")},
    {Distance(115,     Kilometers), 115,   Kilometers, "115",    MakeDistanceStr("115", "km")},
    {Distance(999,     Kilometers), 999,   Kilometers, "999",    MakeDistanceStr("999", "km")},
    {Distance(1000,    Kilometers), 1000,  Kilometers, "1000",   MakeDistanceStr("1000", "km")},
    {Distance(1049.99, Kilometers), 1050,  Kilometers, "1050",   MakeDistanceStr("1050", "km")},
    {Distance(1050,    Kilometers), 1050,  Kilometers, "1050",   MakeDistanceStr("1050", "km")},
    {Distance(1050.01, Kilometers), 1050,  Kilometers, "1050",   MakeDistanceStr("1050", "km")},
    {Distance(1234,    Kilometers), 1234,  Kilometers, "1234",   MakeDistanceStr("1234", "km")},
    {Distance(12345,   Kilometers), 12345, Kilometers, "12,345", MakeDistanceStr("12,345", "km")},

    // From Feet to Feet
    {Distance(0,       Feet),       0,     Feet,       "0",      MakeDistanceStr("0", "ft")},
    {Distance(1,       Feet),       1,     Feet,       "1",      MakeDistanceStr("1", "ft")},
    {Distance(9.99,    Feet),       10,    Feet,       "10",     MakeDistanceStr("10", "ft")},
    {Distance(10.01,   Feet),       10,    Feet,       "10",     MakeDistanceStr("10", "ft")},
    {Distance(95,      Feet),       95,    Feet,       "95",     MakeDistanceStr("95", "ft")},
    {Distance(125,     Feet),       130,   Feet,       "130",    MakeDistanceStr("130", "ft")},
    {Distance(991,     Feet),       990,   Feet,       "990",    MakeDistanceStr("990", "ft")},

    // From Miles to Miles
    {Distance(0,       Miles),      0,     Feet,       "0",      MakeDistanceStr("0", "ft")},
    {Distance(0.1,     Miles),      530,   Feet,       "530",    MakeDistanceStr("530", "ft")},
    {Distance(1,       Miles),      1.0,   Miles,      "1.0",    MakeDistanceStr("1.0", "mi")},
    {Distance(1.234,   Miles),      1.2,   Miles,      "1.2",    MakeDistanceStr("1.2", "mi")},
    {Distance(9.99,    Miles),      10,    Miles,      "10",     MakeDistanceStr("10", "mi")},
    {Distance(10.01,   Miles),      10,    Miles,      "10",     MakeDistanceStr("10", "mi")},
    {Distance(11,      Miles),      11,    Miles,      "11",     MakeDistanceStr("11", "mi")},
    {Distance(54,      Miles),      54,    Miles,      "54",     MakeDistanceStr("54", "mi")},
    {Distance(145,     Miles),      145,   Miles,      "145",    MakeDistanceStr("145", "mi")},
    {Distance(999,     Miles),      999,   Miles,      "999",    MakeDistanceStr("999", "mi")},
    {Distance(1149.99, Miles),      1150,  Miles,      "1150",   MakeDistanceStr("1150", "mi")},
    {Distance(1150,    Miles),      1150,  Miles,      "1150",   MakeDistanceStr("1150", "mi")},
    {Distance(1150.01, Miles),      1150,  Miles,      "1150",   MakeDistanceStr("1150", "mi")},
    {Distance(12345.0, Miles),      12345, Miles,      "12,345", MakeDistanceStr("12,345", "mi")},

    // From Meters to Kilometers
    {Distance(999,     Meters),     1.0,   Kilometers, "1.0",    MakeDistanceStr("1.0", "km")},
    {Distance(1000,    Meters),     1.0,   Kilometers, "1.0",    MakeDistanceStr("1.0", "km")},
    {Distance(1001,    Meters),     1.0,   Kilometers, "1.0",    MakeDistanceStr("1.0", "km")},
    {Distance(1100,    Meters),     1.1,   Kilometers, "1.1",    MakeDistanceStr("1.1", "km")},
    {Distance(1140,    Meters),     1.1,   Kilometers, "1.1",    MakeDistanceStr("1.1", "km")},
    {Distance(1151,    Meters),     1.2,   Kilometers, "1.2",    MakeDistanceStr("1.2", "km")},
    {Distance(1500,    Meters),     1.5,   Kilometers, "1.5",    MakeDistanceStr("1.5", "km")},
    {Distance(1549.9,  Meters),     1.5,   Kilometers, "1.5",    MakeDistanceStr("1.5", "km")},
    {Distance(1550,    Meters),     1.6,   Kilometers, "1.6",    MakeDistanceStr("1.6", "km")},
    {Distance(1551,    Meters),     1.6,   Kilometers, "1.6",    MakeDistanceStr("1.6", "km")},
    {Distance(9949,    Meters),     9.9,   Kilometers, "9.9",    MakeDistanceStr("9.9", "km")},
    {Distance(9992,    Meters),     10,    Kilometers, "10",     MakeDistanceStr("10", "km")},
    {Distance(10000,   Meters),     10,    Kilometers, "10",     MakeDistanceStr("10", "km")},
    {Distance(10499.9, Meters),     10,    Kilometers, "10",     MakeDistanceStr("10", "km")},
    {Distance(10501,   Meters),     11,    Kilometers, "11",     MakeDistanceStr("11", "km")},
    {Distance(101'001, Meters),     101,   Kilometers, "101",    MakeDistanceStr("101", "km")},
    {Distance(101'999, Meters),     102,   Kilometers, "102",    MakeDistanceStr("102", "km")},
    {Distance(287'386, Meters),     287,   Kilometers, "287",    MakeDistanceStr("287", "km")},

    // From Feet to Miles
    {Distance(999,     Feet),       0.2,   Miles,      "0.2",    MakeDistanceStr("0.2", "mi")},
    {Distance(1000,    Feet),       0.2,   Miles,      "0.2",    MakeDistanceStr("0.2", "mi")},
    {Distance(1150,    Feet),       0.2,   Miles,      "0.2",    MakeDistanceStr("0.2", "mi")},
    {Distance(5280,    Feet),       1.0,   Miles,      "1.0",    MakeDistanceStr("1.0", "mi")},
    {Distance(7920,    Feet),       1.5,   Miles,      "1.5",    MakeDistanceStr("1.5", "mi")},
    {Distance(10560,   Feet),       2.0,   Miles,      "2.0",    MakeDistanceStr("2.0", "mi")},
    {Distance(100'000, Feet),       19,    Miles,      "19",     MakeDistanceStr("19", "mi")},
    {Distance(285'120, Feet),       54,    Miles,      "54",     MakeDistanceStr("54", "mi")},
    {Distance(633'547, Feet),       120,   Miles,      "120",    MakeDistanceStr("120", "mi")},
    {Distance(633'600, Feet),       120,   Miles,      "120",    MakeDistanceStr("120", "mi")},
    {Distance(633'653, Feet),       120,   Miles,      "120",    MakeDistanceStr("120", "mi")},
    {Distance(999'999, Feet),       189,   Miles,      "189",    MakeDistanceStr("189", "mi")},
  };

  // clang-format on
  for (auto const & [distance, formattedDistance, formattedUnits, formattedDistanceString, formattedString] : testData)
  {
    Distance const formatted = distance.GetFormattedDistance();
    // Run two times to verify that nothing breaks after second format
    for (auto const & d : {formatted, formatted.GetFormattedDistance()})
    {
      TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), formattedDistance, (distance));
      TEST_EQUAL(d.GetUnits(), formattedUnits, (distance));
      TEST_EQUAL(d.GetDistanceString(), formattedDistanceString, (distance));
      TEST_EQUAL(d.ToString(), formattedString, (distance));
    }
  }
}

}  // namespace platform
