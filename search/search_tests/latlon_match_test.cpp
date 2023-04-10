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

  TEST(MatchLatLonDegree("-22.3534 -42.7076\n", lat, lon), ());
  TestAlmostEqual(lat, -22.3534);
  TestAlmostEqual(lon, -42.7076);

  // The ".123" form is not accepted, so our best-effort
  // parse results in "10" and "20".
  TEST(MatchLatLonDegree(".10, ,20", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(!MatchLatLonDegree("34-31", lat, lon), ());
  TEST(!MatchLatLonDegree("34/31", lat, lon), ());
  TEST(!MatchLatLonDegree("34,31", lat, lon), ());

  TEST(!MatchLatLonDegree("N5E-5", lat, lon), ());
  TEST(!MatchLatLonDegree("5E-5", lat, lon), ());
  TEST(!MatchLatLonDegree("N5W-5", lat, lon), ());
  TEST(!MatchLatLonDegree("5 E-5", lat, lon), ());

  TEST(!MatchLatLonDegree("., .", lat, lon), ());
  TEST(!MatchLatLonDegree("10, .", lat, lon), ());

  TEST(MatchLatLonDegree("0*30\', 1*0\'30\"", lat, lon), ());
  TestAlmostEqual(lat, 0.5);
  TestAlmostEqual(lon, 1.00833333333333);

  TEST(MatchLatLonDegree("50  *, 40 *", lat, lon), ());
  TestAlmostEqual(lat, 50.0);
  TestAlmostEqual(lon, 40.0);

  TEST(!MatchLatLonDegree("50* 40*, 30*", lat, lon), ());

  TEST(MatchLatLonDegree("(S50°30\'30\" W49°59\'59\"", lat, lon), ());
  TestAlmostEqual(lat, -50.50833333333333);
  TestAlmostEqual(lon, -49.99972222222222);

  TEST(MatchLatLonDegree("60* 0\' 0\" N, 0* 0\' 30\" E", lat, lon), ()); // full form, case #1
  TestAlmostEqual(lat, 60);
  TestAlmostEqual(lon, 0.0083333333333333332);

  TEST(MatchLatLonDegree("60°, 30\"", lat, lon), ()); // short form, case #1
  TestAlmostEqual(lat, 60);
  TestAlmostEqual(lon, 0.0083333333333333332);

  TEST(MatchLatLonDegree("0* 50\' 0\" N, 50° 0\' 0\" E", lat, lon), ()); // full form, case #2
  TestAlmostEqual(lat, 0.83333333333333337);
  TestAlmostEqual(lon, 50);

  TEST(MatchLatLonDegree("50\', 50°", lat, lon), ()); // short form, case #2
  TestAlmostEqual(lat, 0.83333333333333337);
  TestAlmostEqual(lon, 50);

  TEST(!MatchLatLonDegree("60° 30\"", lat, lon), ()); // if short form is ambiguous, latitude and longitude
                                                      // should be comma separated

  TEST(!MatchLatLonDegree("-90*50\'50\", -50°", lat, lon), ());

  TEST(MatchLatLonDegree("-10,12 -20,12", lat, lon), ());
  TestAlmostEqual(lat, -10.12);
  TestAlmostEqual(lon, -20.12);

  TEST(MatchLatLonDegree("-89, 180.5555", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, -179.4445);

  TEST(MatchLatLonDegree("-89, 181", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, -179.0);

  TEST(MatchLatLonDegree("-89, 359", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, -1.0);


  TEST(MatchLatLonDegree("-89, 359.5555", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, -0.4445);

  TEST(MatchLatLonDegree("-89, 359.5555", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, -0.4445);

  TEST(MatchLatLonDegree("-89, 360", lat, lon), ());
  TestAlmostEqual(lat, -89.0);
  TestAlmostEqual(lon, 0.0);

  // DMS edge cases
  TEST(MatchLatLonDegree("N 89* 59\' 59.99990\", E 179* 59\' 59.99990\"", lat, lon), ());
  TestAlmostEqual(lat, 89.9999999722222);
  TestAlmostEqual(lon, 179.999999972222);

  TEST(MatchLatLonDegree("N 89* 59\' 59.99999\", E 179* 59\' 59.99999\"", lat, lon), ());
  TestAlmostEqual(lat, 89.9999999972222);
  TestAlmostEqual(lon, 179.999999997222);

  TEST(MatchLatLonDegree("N 90* 0\' 0\", E 180* 0\' 0\"", lat, lon), ());
  TestAlmostEqual(lat, 90.0);
  TestAlmostEqual(lon, 180.0);

  // Fail if degree is negative and minute is floating point
  TEST(!MatchLatLonDegree("-89*15.5\' N; 120*30\'50.5\" e", lat, lon), ());

  TEST(MatchLatLonDegree("N55°45′20.99″ E37°37′03.62″", lat, lon), ());
  TestAlmostEqual(lat, 55.755830555555556);
  TestAlmostEqual(lon, 37.617672222222222);

  TEST(MatchLatLonDegree("S55°45′20.99″ W37°37′03.62″", lat, lon), ());
  TestAlmostEqual(lat, -55.755830555555556);
  TestAlmostEqual(lon, -37.617672222222222);

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

  TEST(!MatchLatLonDegree("54° 25' 0N 1° 53' 46W", lat, lon), ()); // incomplete form

  TEST(MatchLatLonDegree("47.33471° 8.53112°", lat, lon), ());
  TestAlmostEqual(lat, 47.33471);
  TestAlmostEqual(lon, 8.53112);

  TEST(MatchLatLonDegree("N 51* 0\' 33.217\" E 11* 0\' 10.113\"", lat, lon), ());
  TestAlmostEqual(lat, 51.009226944444443);
  TestAlmostEqual(lon, 11.002809166666667);

  TEST(!MatchLatLonDegree("N 51* 33.217 E 11* 60.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 51* -33.217 E 11* 10.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 33.217\' E 11* 10.113", lat, lon), ());
  TEST(!MatchLatLonDegree("N 51* 33.217 E 11* 10.113\"", lat, lon), ());

  // Degrees Decimal Minutes (DDM) tests
  TEST(MatchLatLonDegree("N 51* 33.217\' E 11* 10.113\'", lat, lon), ());
  TestAlmostEqual(lat, 51.55361666666667);
  TestAlmostEqual(lon, 11.16855);

  TEST(MatchLatLonDegree("57* 53.9748\' N, 59* 56.8122\' E", lat, lon), ());
  TestAlmostEqual(lat, 57.89958);
  TestAlmostEqual(lon, 59.94687);

  TEST(MatchLatLonDegree("N 57* 53.9748\', E 59* 56.8122\'", lat, lon), ());
  TestAlmostEqual(lat, 57.89958);
  TestAlmostEqual(lon, 59.94687);

  TEST(MatchLatLonDegree("S 57* 53.9748\', E 59* 56.8122\'", lat, lon), ());
  TestAlmostEqual(lat, -57.89958);
  TestAlmostEqual(lon, 59.94687);

  TEST(MatchLatLonDegree("N 57* 53.9748\', W 59* 56.8122\'", lat, lon), ());
  TestAlmostEqual(lat, 57.89958);
  TestAlmostEqual(lon, -59.94687);

  // DDM edge cases
  TEST(MatchLatLonDegree("N 89* 59,9999990\', E 179* 59,9999990\'", lat, lon), ());
  TestAlmostEqual(lat,  89.999999983333);
  TestAlmostEqual(lon, 179.999999983333);

  TEST(MatchLatLonDegree("N 89* 59,9999999\', E 179* 59,9999999\'", lat, lon), ());
  TestAlmostEqual(lat,  89.999999998333);
  TestAlmostEqual(lon, 179.999999998333);

  TEST(MatchLatLonDegree("N 90* 0\', E 180* 0\'", lat, lon), ());
  TestAlmostEqual(lat,  90.0);
  TestAlmostEqual(lon, 180.0);


  // Fail if cardinal direction marks is same
  TEST(!MatchLatLonDegree("57* 53\' 58.488\"N, 59* 56\' 48.732\"N", lat, lon), ());
  TEST(!MatchLatLonDegree("57* 53\' 58.488\"S, 59* 56\' 48.732\"S", lat, lon), ());
  TEST(!MatchLatLonDegree("57* 53\' 58.488\"E, 59* 56\' 48.732\"E", lat, lon), ());
  TEST(!MatchLatLonDegree("57* 53\' 58.488\"W, 59* 56\' 48.732\"W", lat, lon), ());


  // Fail if degree value is floating point (DMS and DDM notation)
  TEST(!MatchLatLonDegree("57.123* 53\' 58.488\"N, 59* 56\' 48.732\"E", lat, lon), ());
  TEST(!MatchLatLonDegree("57.123* 58.488\'N, 59* 48.732\'E", lat, lon), ());

  // Fail if minute value is floating point (for DMS notation)
  TEST(!MatchLatLonDegree("57* 53.123\' 58.488\"N, 59* 56.123\' 48.732\"E", lat, lon), ());

  // Fail if minute value is greater than 59 (for DMS notation)
  TEST(!MatchLatLonDegree("57* 60\' 58.488\" N, 59* 56\' 48.732\" E", lat, lon), ());

  // Fail if degree value is greater than 180 (DMS and DDM notation)
  TEST(!MatchLatLonDegree("N 89* 10\' 3\", E 183* 3\' 3\"", lat, lon), ());
  TEST(!MatchLatLonDegree("N 89* 10\', E 183* 3\'", lat, lon), ());
}
}  //  namespace
