#include "testing/testing.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"


using namespace MeasurementUtils;

UNIT_TEST(Measurement_Smoke)
{
  Settings::Set("Units", Settings::Metric);

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
  Settings::Set("Units", Settings::Foot);
  TEST_EQUAL(FormatAltitude(10000), "32808ft", ());
  Settings::Set("Units", Settings::Metric);
  TEST_EQUAL(FormatAltitude(5), "5m", ());
}

UNIT_TEST(FormatSpeed)
{
  Settings::Set("Units", Settings::Metric);
  TEST_EQUAL(FormatSpeed(10), "36km/h", ());
  TEST_EQUAL(FormatSpeed(1), "3.6km/h", ());
}
