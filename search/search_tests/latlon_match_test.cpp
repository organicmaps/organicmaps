#include "testing/testing.hpp"

#include "search/latlon_match.hpp"

#include "base/math.hpp"

using namespace search;

namespace
{
// We expect the results to be quite precise.
double const kEps = 1e-12;

void TestAlmostEqual(double actual, double expected)
{
  TEST(base::AlmostEqualAbsOrRel(actual, expected, kEps), (actual, expected));
}

UNIT_TEST(LatLon_Degree_Match)
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

  // The ".123" form is not accepted, so our best-effort
  // parse results in "10" and "20".
  TEST(MatchLatLonDegree(".10, ,20", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(!MatchLatLonDegree("., .", lat, lon), ());
  TEST(!MatchLatLonDegree("10, .", lat, lon), ());

  TEST(MatchLatLonDegree("0*30\', 1*0\'30\"", lat, lon), ());
  TestAlmostEqual(lat, 0.5);
  TestAlmostEqual(lon, 1.00833333333333);

  TEST(MatchLatLonDegree("50  *, 40 *", lat, lon), ());
  TestAlmostEqual(lat, 50.0);
  TestAlmostEqual(lon, 40.0);

  TEST(!MatchLatLonDegree("50* 40*, 30*", lat, lon), ());

  TEST(MatchLatLonDegree("(-50°30\'30\" -49°59\'59\"", lat, lon), ());
  TestAlmostEqual(lat, -50.50833333333333);
  TestAlmostEqual(lon, -49.99972222222222);

  TEST(!MatchLatLonDegree("50°, 30\"", lat, lon), ());
  TEST(!MatchLatLonDegree("50\', -50°", lat, lon), ());
  TEST(!MatchLatLonDegree("-90*50\'50\", -50°", lat, lon), ());

  TEST(MatchLatLonDegree("(-89*, 360*)", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, 0.0);

  TEST(MatchLatLonDegree("-89*15.5\' N; 120*30\'50.5\" e", lat, lon), ());
  TestAlmostEqual(lat, -89.25833333333333);
  TestAlmostEqual(lon, 120.51402777777778);

  TEST(MatchLatLonDegree("N55°45′20.99″ E37°37′03.62″", lat, lon), ());
  TestAlmostEqual(lat, 55.755830555555556);
  TestAlmostEqual(lon, 37.617672222222222);

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
}  //  namespace
