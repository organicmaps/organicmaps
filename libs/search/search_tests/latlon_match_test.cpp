#include "testing/testing.hpp"

#include "search/latlon_match.hpp"

#include "base/math.hpp"

namespace latlon_match_test
{
using namespace search;

// We expect the results to be quite precise.
double const kEps = 1e-12;

void TestAlmostEqual(double actual, double expected)
{
  TEST(AlmostEqualAbsOrRel(actual, expected, kEps), (actual, expected));
}

UNIT_TEST(LatLon_Match_Smoke)
{
  double lat, lon;

  TEST(!MatchLatLonDegree("10,20", lat, lon), ());

  TEST(MatchLatLonDegree("10, 20", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(MatchLatLonDegree("10.0 20.0", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(MatchLatLonDegree("10.0, 20,0", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(MatchLatLonDegree("10.10, 20.20", lat, lon), ());
  TestAlmostEqual(lat, 10.1);
  TestAlmostEqual(lon, 20.2);

  TEST(MatchLatLonDegree("10,10 20,20", lat, lon), ());
  TestAlmostEqual(lat, 10.1);
  TestAlmostEqual(lon, 20.2);

  TEST(MatchLatLonDegree("10,10, 20,20", lat, lon), ());
  TestAlmostEqual(lat, 10.1);
  TestAlmostEqual(lon, 20.2);

  TEST(MatchLatLonDegree("-10,10, 20,20", lat, lon), ());
  TestAlmostEqual(lat, -10.1);
  TestAlmostEqual(lon, 20.2);

  TEST(MatchLatLonDegree("10,10, -20,20", lat, lon), ());
  TestAlmostEqual(lat, 10.1);
  TestAlmostEqual(lon, -20.2);

  TEST(MatchLatLonDegree("-10,10, -20,20", lat, lon), ());
  TestAlmostEqual(lat, -10.1);
  TestAlmostEqual(lon, -20.2);

  TEST(MatchLatLonDegree("-10,10 20,20", lat, lon), ());
  TestAlmostEqual(lat, -10.1);
  TestAlmostEqual(lon, 20.2);

  TEST(MatchLatLonDegree("10,10 -20,20", lat, lon), ());
  TestAlmostEqual(lat, 10.1);
  TestAlmostEqual(lon, -20.2);

  TEST(MatchLatLonDegree("-10,10 -20,20", lat, lon), ());
  TestAlmostEqual(lat, -10.1);
  TestAlmostEqual(lon, -20.2);

  TEST(MatchLatLonDegree("-22.3534 -42.7076\n", lat, lon), ());
  TestAlmostEqual(lat, -22.3534);
  TestAlmostEqual(lon, -42.7076);

  TEST(MatchLatLonDegree("-22,3534 -42,7076\n", lat, lon), ());
  TestAlmostEqual(lat, -22.3534);
  TestAlmostEqual(lon, -42.7076);

  // The ".123" form is not accepted, so our best-effort
  // parse results in "10" and "20".
  TEST(MatchLatLonDegree(".10, ,20", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(!MatchLatLonDegree("34-31", lat, lon), ());
  TEST(!MatchLatLonDegree("34/31", lat, lon), ());

  TEST(!MatchLatLonDegree("N5W-5", lat, lon), ());
  TEST(MatchLatLonDegree("5N5W", lat, lon), ());
  TestAlmostEqual(lat, 5.0);
  TestAlmostEqual(lon, -5.0);

  TEST(MatchLatLonDegree("S5E5", lat, lon), ());
  TestAlmostEqual(lat, -5.0);
  TestAlmostEqual(lon, 5.0);

  TEST(MatchLatLonDegree("90 180", lat, lon), ());
  TestAlmostEqual(lat, 90.0);
  TestAlmostEqual(lon, 180.0);

  TEST(MatchLatLonDegree("-90 -180", lat, lon), ());
  TestAlmostEqual(lat, -90.0);
  TestAlmostEqual(lon, -180.0);

  TEST(MatchLatLonDegree("90 -180", lat, lon), ());
  TestAlmostEqual(lat, 90.0);
  TestAlmostEqual(lon, -180.0);

  TEST(MatchLatLonDegree("-90 180", lat, lon), ());
  TestAlmostEqual(lat, -90.0);
  TestAlmostEqual(lon, 180.0);

  TEST(MatchLatLonDegree("0 0", lat, lon), ());
  TestAlmostEqual(lat, 0.0);
  TestAlmostEqual(lon, 0.0);

  TEST(!MatchLatLonDegree("91 0", lat, lon), ());
  TEST(!MatchLatLonDegree("-91 0", lat, lon), ());
  TEST(!MatchLatLonDegree("0 361", lat, lon), ());
  TEST(!MatchLatLonDegree("0 -181", lat, lon), ());

  TEST(MatchLatLonDegree("0 360", lat, lon), ());
  TestAlmostEqual(lat, 0.0);
  TestAlmostEqual(lon, 0.0);

  TEST(MatchLatLonDegree("0 270", lat, lon), ());
  TestAlmostEqual(lat, 0.0);
  TestAlmostEqual(lon, -90.0);

  TEST(MatchLatLonDegree("0 181", lat, lon), ());
  TestAlmostEqual(lat, 0.0);
  TestAlmostEqual(lon, -179.0);

  TEST(MatchLatLonDegree("1:2:3N 4:5:6E", lat, lon), ());
  TestAlmostEqual(lat, 1.0 + 2.0 / 60.0 + 3.0 / 3600.0);
  TestAlmostEqual(lon, 4.0 + 5.0 / 60.0 + 6.0 / 3600.0);

  TEST(MatchLatLonDegree("1:2:3 4:5:6", lat, lon), ());
  TestAlmostEqual(lat, 1.0 + 2.0 / 60.0 + 3.0 / 3600.0);
  TestAlmostEqual(lon, 4.0 + 5.0 / 60.0 + 6.0 / 3600.0);

  TEST(MatchLatLonDegree("0*30\'30\" -0*30\'30\"", lat, lon), ());
  TestAlmostEqual(lat, 0.5083333333333333);
  TestAlmostEqual(lon, -0.5083333333333333);

  TEST(MatchLatLonDegree("N55°45′20.99″ E37°37′03.62″", lat, lon), ());
  TestAlmostEqual(lat, 55.755830555555556);
  TestAlmostEqual(lon, 37.617672222222222);

  {
    TEST(MatchLatLonDegree("N-55°45′20.99″ E-37°37′03.62″", lat, lon), ());
    double lat1, lon1;
    TEST(MatchLatLonDegree("S55°45′20.99″ W37°37′03.62″", lat1, lon1), ());
    TestAlmostEqual(lat, lat1);
    TestAlmostEqual(lon, lon1);
  }

  TEST(MatchLatLonDegree("55°45’20.9916\"N, 37°37’3.6228\"E hsdfjgkdsjbv", lat, lon), ());
  TestAlmostEqual(lat, 55.755831);
  TestAlmostEqual(lon, 37.617673);

  TEST(MatchLatLonDegree("55°45′20.9916″S, 37°37′3.6228″W", lat, lon), ());
  TestAlmostEqual(lat, -55.755831);
  TestAlmostEqual(lon, -37.617673);

  // We can receive already normalized string, and double quotes become two single quotes
  TEST(MatchLatLonDegree("55°45′20.9916′′S, 37°37′3.6228′′W", lat, lon), ());
  TestAlmostEqual(lat, -55.755831);
  TestAlmostEqual(lon, -37.617673);

  TEST(MatchLatLonDegree("W55°45′20.9916″, S37°37′3.6228″", lat, lon), ());
  TestAlmostEqual(lon, -55.755831);
  TestAlmostEqual(lat, -37.617673);

  TEST(MatchLatLonDegree("55°45′20.9916″ W 37°37′3.6228″ N", lat, lon), ());
  TestAlmostEqual(lon, -55.755831);
  TestAlmostEqual(lat, 37.617673);

  TEST(!MatchLatLonDegree("55°45′20.9916″W 37°37′3.6228″E", lat, lon), ());
  TEST(!MatchLatLonDegree("N55°45′20.9916″ S37°37′3.6228″", lat, lon), ());

  TEST(MatchLatLonDegree("54° 25' 0N 1° 53' 46W", lat, lon), ());
  TestAlmostEqual(lat, 54.41666666666667);
  TestAlmostEqual(lon, -1.89611111111111);

  TEST(MatchLatLonDegree("47.33471°N 8.53112°E", lat, lon), ());
  TestAlmostEqual(lat, 47.33471);
  TestAlmostEqual(lon, 8.53112);

  TEST(MatchLatLonDegree("N 51* 33.217 E 11* 10.113", lat, lon), ());
  TestAlmostEqual(lat, 51.55361666666667);
  TestAlmostEqual(lon, 11.16855);

  TEST(!MatchLatLonDegree("N 51* 33.217 E 11* 60.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 51* -33.217 E 11* 10.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 33.217\' E 11* 10.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 51* 33.217 E 11* 10.113\"", lat, lon), ());
}

UNIT_TEST(LatLon_Match_False)
{
  double lat, lon;
  TEST(!MatchLatLonDegree("2 1st", lat, lon), ());
}

// Space-separated DMS (no degree/minute/second symbols), phone-friendly.
UNIT_TEST(LatLon_Match_SpaceSeparatedDMS)
{
  double lat, lon;

  TEST(MatchLatLonDegree("37 00 27.73 N 08 56 41.59 W", lat, lon), ());
  TestAlmostEqual(lat, 37.0 + 27.73 / 3600.0);
  TestAlmostEqual(lon, -(8.0 + 56.0 / 60.0 + 41.59 / 3600.0));

  // Direction-first.
  TEST(MatchLatLonDegree("N 37 00 27.73 W 08 56 41.59", lat, lon), ());
  TestAlmostEqual(lat, 37.0 + 27.73 / 3600.0);
  TestAlmostEqual(lon, -(8.0 + 56.0 / 60.0 + 41.59 / 3600.0));

  // D M only (no seconds).
  TEST(MatchLatLonDegree("51 30 N 0 7 W", lat, lon), ());
  TestAlmostEqual(lat, 51.5);
  TestAlmostEqual(lon, -7.0 / 60.0);

  // Comma as group delimiter.
  TEST(MatchLatLonDegree("N 37 00 27.73, W 08 56 41.59", lat, lon), ());
  TestAlmostEqual(lat, 37.0 + 27.73 / 3600.0);
  TestAlmostEqual(lon, -(8.0 + 56.0 / 60.0 + 41.59 / 3600.0));

  // Decimal degrees with suffix hemispheres still work (legacy or new).
  TEST(MatchLatLonDegree("51.5 N, 11.2 E", lat, lon), ());
  TestAlmostEqual(lat, 51.5);
  TestAlmostEqual(lon, 11.2);

  // Multi-dot garbage must not match.
  TEST(!MatchLatLonDegree("1.2.3 N 4.5.6 E", lat, lon), ());

  // Ambiguous four-number run must not pick a bad split.
  TEST(!MatchLatLonDegree("N 51 30 0 7 W", lat, lon), ());

  // Extra trailing text must not match the new grammar (legacy may still).
  // "West Street" must not be treated as hemisphere + leftover.
  TEST(!MatchLatLonDegree("51 30 N 0 7 West Street", lat, lon), ());

  // Same-axis rejected.
  TEST(!MatchLatLonDegree("51 30 N 0 7 N", lat, lon), ());

  // Incomplete.
  TEST(!MatchLatLonDegree("51 30 N", lat, lon), ());
}

}  // namespace latlon_match_test
