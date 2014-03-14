#include "../../testing/testing.hpp"
#include "../latlon_match.hpp"
#include "../../std/utility.hpp"

namespace
{

pair<pair<double, double>, pair<double, double> > R(double lat, double lon,
                                                    double precLat, double precLon)
{
  return make_pair(make_pair(lat, lon), make_pair(precLat, precLon));
}

pair<pair<double, double>, pair<double, double> > TestLatLonMatchSuccessful(string const & s)
{
  double lat = -3501;
  double lon = -3502;
  double precLat = -3503;
  double precLon = -3504;
  TEST(search::MatchLatLon(s, lat, lon, precLat, precLon), (s, lat, lon, precLat, precLon));
  return make_pair(make_pair(lat, lon), make_pair(precLat, precLon));
}

void TestLatLonFailed(string const & s)
{
  double lat = -3501;
  double lon = -3502;
  double latPrec = -3503;
  double lonPrec = -3504;
  TEST(!search::MatchLatLon(s, lat, lon, latPrec, lonPrec), (s, lat, lon, latPrec, lonPrec));
  TEST_EQUAL(lat, -3501, ());
  TEST_EQUAL(lon, -3502, ());
  TEST_EQUAL(latPrec, -3503, ());
  TEST_EQUAL(lonPrec, -3504, ());
}

}  // unnamed namespace

UNIT_TEST(LatLon_Match)
{
  TestLatLonFailed("");
  TestLatLonFailed("0.0");
  TestLatLonFailed("2.0 sadas 2.0");
  TestLatLonFailed("90.1 0.0");
  TestLatLonFailed("-90.1 0.0");
  TestLatLonFailed("0.0 361");
  TestLatLonFailed("0.0 -181.0");

  TEST_EQUAL(R(2.0, 3.0, 0.05, 0.05), TestLatLonMatchSuccessful("2.0 3.0"), ());
  TEST_EQUAL(R(-2.0, 3.0, 0.05, 0.05), TestLatLonMatchSuccessful("-2.0 3.0"), ());
  TEST_EQUAL(R(2.0, 3.0, 0.05, 0.05), TestLatLonMatchSuccessful("2.0, 3.0"), ());
  TEST_EQUAL(R(2.0, 3.0, 0.05, 0.05), TestLatLonMatchSuccessful("2.0;3.0"), ());
  TEST_EQUAL(R(2.0, 3.0, 0.5, 0.05), TestLatLonMatchSuccessful("2;3.0"), ());
  TEST_EQUAL(R(-2.0, 3.0, 0.5, 0.05), TestLatLonMatchSuccessful("-2;3.0"), ());
  TEST_EQUAL(R(-2.0, 3.0, 0.5, 0.05), TestLatLonMatchSuccessful("-2.;3.0"), ());
  TEST_EQUAL(R(2.0, 3.03232424, 0.05, 0.000000005), TestLatLonMatchSuccessful("2.0;3.03232424"),());
  TEST_EQUAL(R(2.0, 3.0, 0.5, 0.5), TestLatLonMatchSuccessful("2 3"), ());
  TEST_EQUAL(R(2.0, 3.0, 0.5, 0.5), TestLatLonMatchSuccessful("2 3."), ());
  TEST_EQUAL(R(2.0, 3.0, 0.05, 0.05), TestLatLonMatchSuccessful("(2.0, 3.0)"), ());
  TEST_EQUAL(R(0.0, 0.0, 0.05, 0.05), TestLatLonMatchSuccessful("0.0 0.0"), ());
  TEST_EQUAL(R(0.0, 180.0, 0.05, 0.5), TestLatLonMatchSuccessful("0.0 180"), ());
  TEST_EQUAL(R(0.0, -179.0, 0.05, 0.5), TestLatLonMatchSuccessful("0.0 181"), ());
  TEST_EQUAL(R(0.0, -170.0, 0.05, 0.5), TestLatLonMatchSuccessful("0.0 -170"), ());
  TEST_EQUAL(R(0.0, -180.0, 0.05, 0.5), TestLatLonMatchSuccessful("0.0 -180"), ());
  TEST_EQUAL(R(0.0, -160.0, 0.05, 0.5), TestLatLonMatchSuccessful("0.0 200"), ());
  TEST_EQUAL(R(0.0, 0.0, 0.05, 0.5), TestLatLonMatchSuccessful("0.0 360"), ());
}

UNIT_TEST(LatLon_Degree_Match)
{
  using namespace search;
  double lat, lon;

  TEST(MatchLatLonDegree("0*30\', 1*0\'30\"", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, 0.5, ());
  TEST_ALMOST_EQUAL(lon, 1.00833333333333, ());

  TEST(MatchLatLonDegree("50  *, 40 *", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, 50.0, ());
  TEST_ALMOST_EQUAL(lon, 40.0, ());

  TEST(!MatchLatLonDegree("50* 40*, 30*", lat, lon), ());

  TEST(MatchLatLonDegree("(-50°30\'30\" -49°59\'59\"", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, -50.50833333333333, ());
  TEST_ALMOST_EQUAL(lon, -49.99972222222222, ());

  TEST(!MatchLatLonDegree("50°, 30\"", lat, lon), ());
  TEST(!MatchLatLonDegree("50\', -50°", lat, lon), ());
  TEST(!MatchLatLonDegree("-90*50\'50\", -50°", lat, lon), ());

  TEST(MatchLatLonDegree("(-89*, 360*)", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, -89.0, ());
  TEST_ALMOST_EQUAL(lon, 0.0, ());

  TEST(MatchLatLonDegree("-89*15.5\' N; 120*30\'50.5\" e", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, -89.25833333333333, ());
  TEST_ALMOST_EQUAL(lon, 120.51402777777778, ());

  TEST(MatchLatLonDegree("N55°45′20.99″ E37°37′03.62″", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, 55.755830555555556, ());
  TEST_ALMOST_EQUAL(lon, 37.617672222222222, ());

  TEST(MatchLatLonDegree("55°45’20.9916\"N, 37°37’3.6228\"E hsdfjgkdsjbv", lat, lon), ());
  TEST_ALMOST_EQUAL(lat, 55.755831, ());
  TEST_ALMOST_EQUAL(lon, 37.617673, ());
}
