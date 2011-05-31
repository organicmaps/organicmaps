#include "../../testing/testing.hpp"
#include "../latlon_match.hpp"
#include "../../std/utility.hpp"

namespace
{

pair<double, double> TestLatLonMatchSuccessful(string const & s)
{
  double lat = -3500;
  double lon = -3500;
  TEST(search::MatchLatLon(s, lat, lon), (s, lat, lon));
  return make_pair(lat, lon);
}

void TestLatLonFailed(string const & s)
{
  double lat = -3500;
  double lon = -3500;
  TEST(!search::MatchLatLon(s, lat, lon), (s, lat, lon));
  TEST_EQUAL(lat, -3500, ());
  TEST_EQUAL(lon, -3500, ());
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

  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("2.0 3.0"), ());
  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("2.0, 3.0"), ());
  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("2.0;3.0"), ());
  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("2;3.0"), ());
  TEST_EQUAL(make_pair(2.0, 3.03232424), TestLatLonMatchSuccessful("2.0;3.03232424"), ());
  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("2 3"), ());
  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("2 3."), ());
  TEST_EQUAL(make_pair(2.0, 3.0), TestLatLonMatchSuccessful("(2.0, 3.0)"), ());
  TEST_EQUAL(make_pair(0.0, 0.0), TestLatLonMatchSuccessful("0.0 0.0"), ());
  TEST_EQUAL(make_pair(0.0, 180.0), TestLatLonMatchSuccessful("0.0 180"), ());
  TEST_EQUAL(make_pair(0.0, -179.0), TestLatLonMatchSuccessful("0.0 181"), ());
  TEST_EQUAL(make_pair(0.0, -170.0), TestLatLonMatchSuccessful("0.0 -170"), ());
  TEST_EQUAL(make_pair(0.0, -180.0), TestLatLonMatchSuccessful("0.0 -180"), ());
  TEST_EQUAL(make_pair(0.0, -160.0), TestLatLonMatchSuccessful("0.0 200"), ());
  TEST_EQUAL(make_pair(0.0, 0.0), TestLatLonMatchSuccessful("0.0 360"), ());
}
