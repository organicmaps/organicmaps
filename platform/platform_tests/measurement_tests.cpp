#include "testing/testing.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "base/math.hpp"

#include "std/utility.hpp"

using namespace measurement_utils;
using namespace settings;

struct ScopedSettings
{
  ScopedSettings() { m_wasSet = Get(kMeasurementUnits, m_oldUnits); }

  /// Saves/restores previous units and sets new units for a scope.
  ScopedSettings(Units newUnits) : ScopedSettings()
  {
    Set(kMeasurementUnits, newUnits);
  }

  ~ScopedSettings()
  {
    if (m_wasSet)
      Set(kMeasurementUnits, m_oldUnits);
    else
      Delete(kMeasurementUnits);
  }

  bool m_wasSet;
  Units m_oldUnits;
};

UNIT_TEST(Measurement_Smoke)
{
  ScopedSettings guard(Units::Metric);

  typedef pair<double, char const *> PairT;

  PairT arr[] = {
    PairT(10.0, "10 m"),
    PairT(10.4, "10 m"),
    PairT(10.51, "11 m"),
    PairT(1000.0, "1.0 km"),
    PairT(1100.0, "1.1 km"),
    PairT(1140.0, "1.1 km"),
    PairT(1151.0, "1.2 km"),
    PairT(1500.0, "1.5 km"),
    PairT(1549.9, "1.5 km"),
    PairT(1551.0, "1.6 km"),
    PairT(10000.0, "10 km"),
    PairT(10400.0, "10 km"),
    PairT(10499.9, "10 km"),
    PairT(10501.0, "11 km")
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    string s;
    TEST(FormatDistance(arr[i].first, s), (arr[i]));
    TEST_EQUAL(s, arr[i].second, (arr[i]));
  }
}

UNIT_TEST(LatLonToDMS_Origin)
{
  TEST_EQUAL(FormatLatLonAsDMS(0, 0), "00°00′00″ 00°00′00″", ());
  TEST_EQUAL(FormatLatLonAsDMS(0, 0, 3), "00°00′00″ 00°00′00″", ());
}

UNIT_TEST(LatLonToDMS_Rounding)
{
  // Here and after data is from Wiki: http://bit.ly/datafotformatingtest
  // Boston
  TEST_EQUAL(FormatLatLonAsDMS(42.358056, -71.063611, 0), "42°21′29″N 71°03′49″W", ());
  // Minsk
  TEST_EQUAL(FormatLatLonAsDMS(53.916667, 27.55, 0), "53°55′00″N 27°33′00″E", ());
  // Rio
  TEST_EQUAL(FormatLatLonAsDMS(-22.908333, -43.196389, 0), "22°54′30″S 43°11′47″W", ());
}

UNIT_TEST(LatLonToDMS_NoRounding)
{
  // Paris
  TEST_EQUAL(FormatLatLonAsDMS(48.8567, 2.3508, 2), "48°51′24.12″N 02°21′02.88″E", ());
  // Vatican
  TEST_EQUAL(FormatLatLonAsDMS(41.904, 12.453, 2), "41°54′14.4″N 12°27′10.8″E", ());

  TEST_EQUAL(FormatLatLonAsDMS(21.981112, -159.371112, 2), "21°58′52″N 159°22′16″W", ());
}

UNIT_TEST(FormatAltitude)
{
  ScopedSettings guard;
  settings::Set(settings::kMeasurementUnits, Units::Imperial);
  TEST_EQUAL(FormatAltitude(10000), "32808ft", ());
  settings::Set(settings::kMeasurementUnits, Units::Metric);
  TEST_EQUAL(FormatAltitude(5), "5m", ());
}

UNIT_TEST(FormatSpeedWithDeviceUnits)
{
  {
    ScopedSettings guard(Units::Metric);
    TEST_EQUAL(FormatSpeedWithDeviceUnits(10), "36km/h", ());
    TEST_EQUAL(FormatSpeedWithDeviceUnits(1), "3.6km/h", ());
  }

  {
    ScopedSettings guard(Units::Imperial);
    TEST_EQUAL(FormatSpeedWithDeviceUnits(10), "22mph", ());
    TEST_EQUAL(FormatSpeedWithDeviceUnits(1), "2.2mph", ());
  }
}

UNIT_TEST(FormatSpeedWithUnits)
{
  TEST_EQUAL(FormatSpeedWithUnits(10, Units::Metric), "36km/h", ());
  TEST_EQUAL(FormatSpeedWithUnits(1, Units::Metric), "3.6km/h", ());

  TEST_EQUAL(FormatSpeedWithUnits(10, Units::Imperial), "22mph", ());
  TEST_EQUAL(FormatSpeedWithUnits(1, Units::Imperial), "2.2mph", ());
}

UNIT_TEST(FormatSpeed)
{
  TEST_EQUAL(FormatSpeed(10, Units::Metric), "36", ());
  TEST_EQUAL(FormatSpeed(1, Units::Metric), "3.6", ());

  TEST_EQUAL(FormatSpeed(10, Units::Imperial), "22", ());
  TEST_EQUAL(FormatSpeed(1, Units::Imperial), "2.2", ());
}

UNIT_TEST(FormatSpeedUnits)
{
  TEST_EQUAL(FormatSpeedUnits(Units::Metric), "km/h", ());
  TEST_EQUAL(FormatSpeedUnits(Units::Imperial), "mph", ());
}

UNIT_TEST(OSMDistanceToMetersString)
{
  TEST_EQUAL(OSMDistanceToMetersString(""), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("INF"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("NAN"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("not a number"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("10й"), "10", ());
  TEST_EQUAL(OSMDistanceToMetersString("0"), "0", ());
  TEST_EQUAL(OSMDistanceToMetersString("0.0"), "0", ());
  TEST_EQUAL(OSMDistanceToMetersString("0.0000000"), "0", ());
  TEST_EQUAL(OSMDistanceToMetersString("22.5"), "22.5", ());
  TEST_EQUAL(OSMDistanceToMetersString("+21.5"), "21.5", ());
  TEST_EQUAL(OSMDistanceToMetersString("1e+2"), "100", ());
  TEST_EQUAL(OSMDistanceToMetersString("5 метров"), "5", ());
  TEST_EQUAL(OSMDistanceToMetersString(" 4.4 "), "4.4", ());
  TEST_EQUAL(OSMDistanceToMetersString("8-15"), "15", ());
  TEST_EQUAL(OSMDistanceToMetersString("8-15 ft"), "4.57", ());
  TEST_EQUAL(OSMDistanceToMetersString("8-й километр"), "8", ());
  // Do not support lists for distance values.
  TEST_EQUAL(OSMDistanceToMetersString("8;9;10"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("8;9;10 meters"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("8;9;10 km"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("10;20!111"), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("10;20\""), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("-100.3"), "-100.3", ());
  TEST_EQUAL(OSMDistanceToMetersString("99.0000000"), "99", ());
  TEST_EQUAL(OSMDistanceToMetersString("8900.000023"), "8900", ());
  TEST_EQUAL(OSMDistanceToMetersString("-300.9999"), "-301", ());
  TEST_EQUAL(OSMDistanceToMetersString("-300.9"), "-300.9", ());
  TEST_EQUAL(OSMDistanceToMetersString("15 m"), "15", ());
  TEST_EQUAL(OSMDistanceToMetersString("15.9 m"), "15.9", ());
  TEST_EQUAL(OSMDistanceToMetersString("15.9m"), "15.9", ());
  TEST_EQUAL(OSMDistanceToMetersString("3000 ft"), "914.4", ());
  TEST_EQUAL(OSMDistanceToMetersString("3000ft"), "914.4", ());
  TEST_EQUAL(OSMDistanceToMetersString("100 feet"), "30.48", ());
  TEST_EQUAL(OSMDistanceToMetersString("100feet"), "30.48", ());
  TEST_EQUAL(OSMDistanceToMetersString("3 nmi"), "5556", ());
  TEST_EQUAL(OSMDistanceToMetersString("3 mi"), "4828.03", ());
  TEST_EQUAL(OSMDistanceToMetersString("3.3 km"), "3300", ());
  TEST_EQUAL(OSMDistanceToMetersString("105'"), "32", ());
  TEST_EQUAL(OSMDistanceToMetersString("11'"), "3.35", ());
  TEST_EQUAL(OSMDistanceToMetersString("11 3\""), "11", ());
  TEST_EQUAL(OSMDistanceToMetersString("11 3'"), "11", ());
  TEST_EQUAL(OSMDistanceToMetersString("11\"'"), "0.28", ());
  TEST_EQUAL(OSMDistanceToMetersString("11'\""), "3.35", ());
  TEST_EQUAL(OSMDistanceToMetersString("11'4\""), "3.45", ());
  TEST_EQUAL(OSMDistanceToMetersString("11\""), "0.28", ());
  TEST_EQUAL(OSMDistanceToMetersString("100 \""), "100", ());

  TEST_EQUAL(OSMDistanceToMetersString("0", false), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("-15", false), "", ());
  TEST_EQUAL(OSMDistanceToMetersString("15.12345", false, 1), "15.1", ());
  TEST_EQUAL(OSMDistanceToMetersString("15.123", false, 4), "15.123", ());
  TEST_EQUAL(OSMDistanceToMetersString("15.654321", true, 1), "15.7", ());
}

UNIT_TEST(UnitsConversion)
{
  double const kEps = 1e-5;
  TEST(base::AlmostEqualAbs(MilesToMeters(MetersToMiles(1000.0)), 1000.0, kEps), ());
  TEST(base::AlmostEqualAbs(MilesToMeters(1.0), 1609.344, kEps), ());

  TEST(base::AlmostEqualAbs(MphToKmph(KmphToMph(100.0)), 100.0, kEps), ());
  TEST(base::AlmostEqualAbs(MphToKmph(100.0), 160.9344, kEps), (MphToKmph(100.0)));

  TEST(base::AlmostEqualAbs(ToSpeedKmPH(100.0, Units::Imperial), 160.9344, kEps), ());
  TEST(base::AlmostEqualAbs(ToSpeedKmPH(100.0, Units::Metric), 100.0, kEps), ());
}
