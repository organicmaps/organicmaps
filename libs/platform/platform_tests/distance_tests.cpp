#include "testing/testing.hpp"

#include "platform/distance.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <algorithm>

namespace platform
{
std::string MakeDistanceStr(std::string value, Distance::Units unit)
{
  static Locale const loc = GetCurrentLocale();

  constexpr char kHardCodedGroupingSeparator = ',';
  if (auto found = value.find(kHardCodedGroupingSeparator); found != std::string::npos)
    value.replace(found, 1, loc.m_groupingSeparator);

  constexpr char kHardCodedDecimalSeparator = '.';
  if (auto found = value.find(kHardCodedDecimalSeparator); found != std::string::npos)
    value.replace(found, 1, loc.m_decimalSeparator);

  return value.append(kNarrowNonBreakingSpace).append(DebugPrint(unit));
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
  using enum Distance::Units;
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    Distance const d = Distance::CreateFormatted(100);
    TEST_EQUAL(d.GetUnits(), Meters, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 100.0, ());
    TEST_EQUAL(d.GetDistanceString(), "100", ());
    TEST_EQUAL(d.ToString(), MakeDistanceStr("100", Meters), ());
  }
  {
    ScopedSettings const guard(measurement_utils::Units::Imperial);

    Distance const d = Distance::CreateFormatted(100);
    TEST_EQUAL(d.GetUnits(), Feet, ());
    TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), 330.0, ());
    TEST_EQUAL(d.GetDistanceString(), "330", ());
    TEST_EQUAL(d.ToString(), MakeDistanceStr("330", Feet), ());
  }
}

UNIT_TEST(Distance_CreateAltitudeFormatted)
{
  using enum Distance::Units;
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    TEST_EQUAL(Distance::FormatAltitude(5), MakeDistanceStr("5", Meters), ());
    TEST_EQUAL(Distance::FormatAltitude(-8849), MakeDistanceStr("-8849", Meters), ());
    TEST_EQUAL(Distance::FormatAltitude(12345), MakeDistanceStr("12,345", Meters), ());
  }
  {
    ScopedSettings const guard(measurement_utils::Units::Imperial);

    TEST_EQUAL(Distance::FormatAltitude(10000), MakeDistanceStr("32,808", Feet), ());
  }
}

UNIT_TEST(Distance_IsLowUnits)
{
  using enum Distance::Units;
  TEST_EQUAL(Distance(0.0, Meters).IsLowUnits(), true, ());
  TEST_EQUAL(Distance(0.0, Feet).IsLowUnits(), true, ());
  TEST_EQUAL(Distance(0.0, Kilometers).IsLowUnits(), false, ());
  TEST_EQUAL(Distance(0.0, Miles).IsLowUnits(), false, ());
}

UNIT_TEST(Distance_IsHighUnits)
{
  using enum Distance::Units;
  TEST_EQUAL(Distance(0.0, Meters).IsHighUnits(), false, ());
  TEST_EQUAL(Distance(0.0, Feet).IsHighUnits(), false, ());
  TEST_EQUAL(Distance(0.0, Kilometers).IsHighUnits(), true, ());
  TEST_EQUAL(Distance(0.0, Miles).IsHighUnits(), true, ());
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
    Distance const formatted = Distance(data.initialDistance, data.initialUnits).To(data.to).GetFormattedDistance();
    TEST_ALMOST_EQUAL_ULPS(formatted.GetDistance(), data.newDistance, (data.initialDistance));
    TEST_EQUAL(formatted.GetUnits(), data.newUnits, ());
  }
}

UNIT_TEST(Distance_ToPlatformUnitsFormatted)
{
  using enum Distance::Units;
  {
    ScopedSettings const guard(measurement_utils::Units::Metric);

    Distance d{11, Feet};
    Distance newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Meters, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 3.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "3", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("3", Meters), (d.ToString()));

    d = Distance{11, Kilometers};
    newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Kilometers, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 11.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "11", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("11", Kilometers), (d.ToString()));
  }

  {
    ScopedSettings const guard(measurement_utils::Units::Imperial);

    Distance d{11, Feet};
    Distance newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Feet, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 11.0, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "11", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("11", Feet), (d.ToString()));

    d = Distance{11, Kilometers};
    newDistance = d.ToPlatformUnitsFormatted();

    TEST_EQUAL(newDistance.GetUnits(), Miles, (d.ToString()));
    TEST_ALMOST_EQUAL_ULPS(newDistance.GetDistance(), 6.8, (d.ToString()));
    TEST_EQUAL(newDistance.GetDistanceString(), "6.8", (d.ToString()));
    TEST_EQUAL(newDistance.ToString(), MakeDistanceStr("6.8", Miles), (d.ToString()));
  }
}

UNIT_TEST(Distance_GetUnits)
{
  using enum Distance::Units;
  TEST_EQUAL(Distance(1234).GetUnits(), Meters, ());
  TEST_EQUAL(Distance(1234, Kilometers).GetUnits(), Kilometers, ());
  TEST_EQUAL(Distance(1234, Feet).GetUnits(), Feet, ());
  TEST_EQUAL(Distance(1234, Miles).GetUnits(), Miles, ());
}

UNIT_TEST(Distance_GetUnitsString)
{
  using enum Distance::Units;
  TEST_EQUAL(Distance(1234).GetUnitsString(), "m", ());
  TEST_EQUAL(Distance(1234, Meters).GetUnitsString(), "m", ());
  TEST_EQUAL(Distance(1234, Kilometers).GetUnitsString(), "km", ());
  TEST_EQUAL(Distance(1234, Feet).GetUnitsString(), "ft", ());
  TEST_EQUAL(Distance(1234, Miles).GetUnitsString(), "mi", ());
}

UNIT_TEST(Distance_FormattedDistance)
{
  struct TestData
  {
    Distance distance;
    double formattedDistance;
    Distance::Units formattedUnits;
    std::string formattedDistanceStringInUsLocale;
  };

  using enum Distance::Units;
  // clang-format off
  TestData const testData[] = {
    // From Meters to Meters
    {Distance(0,       Meters),     0,     Meters,     "0"},
    {Distance(0.3,     Meters),     0,     Meters,     "0"},
    {Distance(0.9,     Meters),     1,     Meters,     "1"},
    {Distance(1,       Meters),     1,     Meters,     "1"},
    {Distance(1.234,   Meters),     1,     Meters,     "1"},
    {Distance(9.99,    Meters),     10,    Meters,     "10"},
    {Distance(10.01,   Meters),     10,    Meters,     "10"},
    {Distance(10.4,    Meters),     10,    Meters,     "10"},
    {Distance(10.5,    Meters),     11,    Meters,     "11"},
    {Distance(10.51,   Meters),     11,    Meters,     "11"},
    {Distance(64.2,    Meters),     64,    Meters,     "64"},
    {Distance(99,      Meters),     99,    Meters,     "99"},
    {Distance(100,     Meters),     100,   Meters,     "100"},
    {Distance(101,     Meters),     100,   Meters,     "100"},
    {Distance(109,     Meters),     110,   Meters,     "110"},
    {Distance(991,     Meters),     990,   Meters,     "990"},

    // From Kilometers to Kilometers
    {Distance(0,       Kilometers), 0,     Meters,     "0"},
    {Distance(0.3,     Kilometers), 300,   Meters,     "300"},
    {Distance(1.234,   Kilometers), 1.2,   Kilometers, "1.2"},
    {Distance(10,      Kilometers), 10,    Kilometers, "10"},
    {Distance(11,      Kilometers), 11,    Kilometers, "11"},
    {Distance(54,      Kilometers), 54,    Kilometers, "54"},
    {Distance(99.99,   Kilometers), 100,   Kilometers, "100"},
    {Distance(100.01,  Kilometers), 100,   Kilometers, "100"},
    {Distance(115,     Kilometers), 115,   Kilometers, "115"},
    {Distance(999,     Kilometers), 999,   Kilometers, "999"},
    {Distance(1000,    Kilometers), 1000,  Kilometers, "1000"},
    {Distance(1049.99, Kilometers), 1050,  Kilometers, "1050"},
    {Distance(1050,    Kilometers), 1050,  Kilometers, "1050"},
    {Distance(1050.01, Kilometers), 1050,  Kilometers, "1050"},
    {Distance(1234,    Kilometers), 1234,  Kilometers, "1234"},
    {Distance(12345,   Kilometers), 12345, Kilometers, "12,345"},

    // From Feet to Feet
    {Distance(0,       Feet),       0,     Feet,       "0"},
    {Distance(1,       Feet),       1,     Feet,       "1"},
    {Distance(9.99,    Feet),       10,    Feet,       "10"},
    {Distance(10.01,   Feet),       10,    Feet,       "10"},
    {Distance(95,      Feet),       95,    Feet,       "95"},
    {Distance(125,     Feet),       130,   Feet,       "130"},
    {Distance(991,     Feet),       990,   Feet,       "990"},

    // From Miles to Miles
    {Distance(0,       Miles),      0,     Feet,       "0"},
    {Distance(0.1,     Miles),      530,   Feet,       "530"},
    {Distance(1,       Miles),      1.0,   Miles,      "1.0"},
    {Distance(1.234,   Miles),      1.2,   Miles,      "1.2"},
    {Distance(9.99,    Miles),      10,    Miles,      "10"},
    {Distance(10.01,   Miles),      10,    Miles,      "10"},
    {Distance(11,      Miles),      11,    Miles,      "11"},
    {Distance(54,      Miles),      54,    Miles,      "54"},
    {Distance(145,     Miles),      145,   Miles,      "145"},
    {Distance(999,     Miles),      999,   Miles,      "999"},
    {Distance(1149.99, Miles),      1150,  Miles,      "1150"},
    {Distance(1150,    Miles),      1150,  Miles,      "1150"},
    {Distance(1150.01, Miles),      1150,  Miles,      "1150"},
    {Distance(12345.0, Miles),      12345, Miles,      "12,345"},

    // From Meters to Kilometers
    {Distance(999,     Meters),     1.0,   Kilometers, "1.0"},
    {Distance(1000,    Meters),     1.0,   Kilometers, "1.0"},
    {Distance(1001,    Meters),     1.0,   Kilometers, "1.0"},
    {Distance(1100,    Meters),     1.1,   Kilometers, "1.1"},
    {Distance(1140,    Meters),     1.1,   Kilometers, "1.1"},
    {Distance(1151,    Meters),     1.2,   Kilometers, "1.2"},
    {Distance(1500,    Meters),     1.5,   Kilometers, "1.5"},
    {Distance(1549.9,  Meters),     1.5,   Kilometers, "1.5"},
    {Distance(1550,    Meters),     1.6,   Kilometers, "1.6"},
    {Distance(1551,    Meters),     1.6,   Kilometers, "1.6"},
    {Distance(9949,    Meters),     9.9,   Kilometers, "9.9"},
    {Distance(9992,    Meters),     10,    Kilometers, "10"},
    {Distance(10000,   Meters),     10,    Kilometers, "10"},
    {Distance(10499.9, Meters),     10,    Kilometers, "10"},
    {Distance(10501,   Meters),     11,    Kilometers, "11"},
    {Distance(101'001, Meters),     101,   Kilometers, "101"},
    {Distance(101'999, Meters),     102,   Kilometers, "102"},
    {Distance(287'386, Meters),     287,   Kilometers, "287"},

    // From Feet to Miles
    {Distance(999,     Feet),       0.2,   Miles,      "0.2"},
    {Distance(1000,    Feet),       0.2,   Miles,      "0.2"},
    {Distance(1150,    Feet),       0.2,   Miles,      "0.2"},
    {Distance(5280,    Feet),       1.0,   Miles,      "1.0"},
    {Distance(7920,    Feet),       1.5,   Miles,      "1.5"},
    {Distance(10560,   Feet),       2.0,   Miles,      "2.0"},
    {Distance(100'000, Feet),       19,    Miles,      "19"},
    {Distance(285'120, Feet),       54,    Miles,      "54"},
    {Distance(633'547, Feet),       120,   Miles,      "120"},
    {Distance(633'600, Feet),       120,   Miles,      "120"},
    {Distance(633'653, Feet),       120,   Miles,      "120"},
    {Distance(999'999, Feet),       189,   Miles,      "189"},
  };

  // clang-format on
  for (auto const & [distance, formattedDistance, formattedUnits, formattedDistanceStringInUsLocale] : testData)
  {
    Distance const formatted = distance.GetFormattedDistance();
    // Run two times to verify that nothing breaks after second format
    for (auto const & d : {formatted, formatted.GetFormattedDistance()})
    {
      TEST_ALMOST_EQUAL_ULPS(d.GetDistance(), formattedDistance, (distance));
      TEST_EQUAL(d.GetUnits(), formattedUnits, (distance));
      auto const formattedString = MakeDistanceStr(formattedDistanceStringInUsLocale, formattedUnits);
      TEST_EQUAL(d.ToString(), formattedString, (distance));
    }
  }
}

}  // namespace platform
