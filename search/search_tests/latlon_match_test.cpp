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

UNIT_TEST(LatLonMatch)
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
