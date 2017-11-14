#include "testing/testing.hpp"

#include "search/latlon_match.hpp"

#include "base/math.hpp"

using namespace search;

UNIT_TEST(LatLon_Degree_Match)
{
  double lat, lon;

  TEST(!MatchLatLonDegree("10,20", lat, lon), ());

  TEST(MatchLatLonDegree("10, 20", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.0, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.0, ());

  TEST(MatchLatLonDegree("10.0 20.0", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.0, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.0, ());

  TEST(MatchLatLonDegree("10.0, 20,0", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.0, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.0, ());

  TEST(MatchLatLonDegree("10.10, 20.20", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.1, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.2, ());

  TEST(MatchLatLonDegree("10,10 20,20", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.1, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.2, ());

  TEST(MatchLatLonDegree("10,10, 20,20", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.1, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.2, ());

  // The ".123" form is not accepted, so our best-effort
  // parse results in "10" and "20".
  TEST(MatchLatLonDegree(".10, ,20", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 10.0, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 20.0, ());

  TEST(!MatchLatLonDegree("., .", lat, lon), ());
  TEST(!MatchLatLonDegree("10, .", lat, lon), ());

  TEST(MatchLatLonDegree("0*30\', 1*0\'30\"", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 0.5, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 1.00833333333333, ());

  TEST(MatchLatLonDegree("50  *, 40 *", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 50.0, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 40.0, ());

  TEST(!MatchLatLonDegree("50* 40*, 30*", lat, lon), ());

  TEST(MatchLatLonDegree("(-50°30\'30\" -49°59\'59\"", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, -50.50833333333333, ());
  TEST_ALMOST_EQUAL_ULPS(lon, -49.99972222222222, ());

  TEST(!MatchLatLonDegree("50°, 30\"", lat, lon), ());
  TEST(!MatchLatLonDegree("50\', -50°", lat, lon), ());
  TEST(!MatchLatLonDegree("-90*50\'50\", -50°", lat, lon), ());

  TEST(MatchLatLonDegree("(-89*, 360*)", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, -89.0, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 0.0, ());

  TEST(MatchLatLonDegree("-89*15.5\' N; 120*30\'50.5\" e", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, -89.25833333333333, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 120.51402777777778, ());

  TEST(MatchLatLonDegree("N55°45′20.99″ E37°37′03.62″", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 55.755830555555556, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 37.617672222222222, ());

  TEST(MatchLatLonDegree("55°45’20.9916\"N, 37°37’3.6228\"E hsdfjgkdsjbv", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 55.755831, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 37.617673, ());

  TEST(MatchLatLonDegree("55°45′20.9916″S, 37°37′3.6228″W", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, -55.755831, ());
  TEST_ALMOST_EQUAL_ULPS(lon, -37.617673, ());

  // We can receive already normalized string, and double quotes become two single quotes
  TEST(MatchLatLonDegree("55°45′20.9916′′S, 37°37′3.6228′′W", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, -55.755831, ());
  TEST_ALMOST_EQUAL_ULPS(lon, -37.617673, ());

  TEST(MatchLatLonDegree("W55°45′20.9916″, S37°37′3.6228″", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lon, -55.755831, ());
  TEST_ALMOST_EQUAL_ULPS(lat, -37.617673, ());

  TEST(MatchLatLonDegree("55°45′20.9916″ W 37°37′3.6228″ N", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lon, -55.755831, ());
  TEST_ALMOST_EQUAL_ULPS(lat, 37.617673, ());

  TEST(!MatchLatLonDegree("55°45′20.9916″W 37°37′3.6228″E", lat, lon), ());
  TEST(!MatchLatLonDegree("N55°45′20.9916″ S37°37′3.6228″", lat, lon), ());

  TEST(MatchLatLonDegree("54° 25' 0N 1° 53' 46W", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 54.41666666666667, ());
  TEST_ALMOST_EQUAL_ULPS(lon, -1.89611111111111, ());

  TEST(MatchLatLonDegree("47.33471°N 8.53112°E", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 47.33471, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 8.53112, ());

  TEST(MatchLatLonDegree("N 51* 33.217 E 11* 10.113", lat, lon), ());
  TEST_ALMOST_EQUAL_ULPS(lat, 51.55361666666667, ());
  TEST_ALMOST_EQUAL_ULPS(lon, 11.16855, ());

  TEST(!MatchLatLonDegree("N 51* 33.217 E 11* 60.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 51* -33.217 E 11* 10.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 33.217\' E 11* 10.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 51* 33.217 E 11* 10.113\"", lat, lon), ());
}
