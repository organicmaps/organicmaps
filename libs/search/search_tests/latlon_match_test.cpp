#include "testing/testing.hpp"

#include "search/latlon_match.hpp"

#include "base/math.hpp"

namespace latlon_match_test
{
using namespace search;

void TestAlmostEqual(double a, double b)
{
  TEST(base::AlmostEqualAbs(a, b, 1.0E-10), (a, b));
}

UNIT_TEST(LatLon_Match_Degree)
{
  double lat, lon;

  TEST(MatchLatLonDegree("10.0 20.0", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  TEST(MatchLatLonDegree("10.0, 20.0", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  // Preserve decimal degrees with suffix hemispheres.
  TEST(MatchLatLonDegree("51.5 N, 11.2 E", lat, lon), ());
  TestAlmostEqual(lat, 51.5);
  TestAlmostEqual(lon, 11.2);
  TEST(!MatchLatLonDegree("1.2.3 N 4.5.6 E", lat, lon), ());

  TEST(MatchLatLonDegree("41.2232 34.2222", lat, lon), ());
  TEST(MatchLatLonDegree("0.1 0.2", lat, lon), ());
  TestAlmostEqual(lat, 0.1);
  TestAlmostEqual(lon, 0.2);

  TEST(MatchLatLonDegree("41.2232N 34.2222E", lat, lon), ());
  TEST(MatchLatLonDegree("41.2232N,34.2222E", lat, lon), ());

  TEST(MatchLatLonDegree("N41.2232 E34.2222", lat, lon), ());

  TEST(MatchLatLonDegree("41.2232 34.2222", lat, lon), ());

  double const lat1 = 45.0 + 31.0 / 60.0 + 25.0 / 3600.0;
  double const lon1 = 9.0 + 19.0 / 60.0 + 14.0 / 3600.0;

  TEST(MatchLatLonDegree("45°31'25,N 9°19'14,E", lat, lon), ());
  TestAlmostEqual(lat, lat1);
  TestAlmostEqual(lon, lon1);

  TEST(MatchLatLonDegree("45°31'25\"N 9°19'14\"E", lat, lon), ());
  TestAlmostEqual(lat, lat1);
  TestAlmostEqual(lon, lon1);

  TEST(MatchLatLonDegree("45°31'25\" 9°19'14\"", lat, lon), ());
  TestAlmostEqual(lat, lat1);
  TestAlmostEqual(lon, lon1);

  TEST(MatchLatLonDegree("45°31'25\"N;9°19'14\"E", lat, lon), ());
  TestAlmostEqual(lat, lat1);
  TestAlmostEqual(lon, lon1);

  TEST(MatchLatLonDegree("45°31'25\"N 009°19'14\"E", lat, lon), ());
  TestAlmostEqual(lat, lat1);
  TestAlmostEqual(lon, lon1);

  TEST(MatchLatLonDegree("45°31.25' 9°19.14'", lat, lon), ());

  TEST(MatchLatLonDegree("45°31'25N 9°19'14E", lat, lon), ());
  TestAlmostEqual(lat, lat1);
  TestAlmostEqual(lon, lon1);

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

UNIT_TEST(LatLon_Match_SpaceSeparatedDMS)
{
  double lat, lon;

  // Phone-keyboard-friendly DMS without degree/minute/second symbols (#13422).
  TEST(MatchLatLonDegree("37 00 27.73 N 08 56 41.59 W", lat, lon), ());
  TestAlmostEqual(lat, 37.00770277777778);
  TestAlmostEqual(lon, -8.94488611111111);

  TEST(MatchLatLonDegree("51 30 0 N 0 7 0 W", lat, lon), ());
  TestAlmostEqual(lat, 51.5);
  TestAlmostEqual(lon, -0.11666666666667);

  // Direction-first groups.
  TEST(MatchLatLonDegree("N 37 00 27.73 W 08 56 41.59", lat, lon), ());
  TestAlmostEqual(lat, 37.00770277777778);
  TestAlmostEqual(lon, -8.94488611111111);

  // Reversed axis order (lon then lat).
  TEST(MatchLatLonDegree("08 56 41.59 W 37 00 27.73 N", lat, lon), ());
  TestAlmostEqual(lat, 37.00770277777778);
  TestAlmostEqual(lon, -8.94488611111111);

  // Two-component D M (no seconds).
  TEST(MatchLatLonDegree("51 30 N 0 7 W", lat, lon), ());
  TestAlmostEqual(lat, 51.5);
  TestAlmostEqual(lon, -0.11666666666667);

  // Decimal degrees must still work (not interpreted as broken DMS).
  TEST(MatchLatLonDegree("10.0 20.0", lat, lon), ());
  TestAlmostEqual(lat, 10.0);
  TestAlmostEqual(lon, 20.0);

  // Range: invalid minutes.
  TEST(!MatchLatLonDegree("51 60 0 N 0 7 0 W", lat, lon), ());
  // Range: latitude > 90.
  TEST(!MatchLatLonDegree("91 0 0 N 0 0 0 E", lat, lon), ());
  // Same-axis hemispheres.
  TEST(!MatchLatLonDegree("51 30 0 N 52 30 0 N", lat, lon), ());
  // Malformed multi-dot components must not match.
  TEST(!MatchLatLonDegree("1.2.3 N 4.5.6 E", lat, lon), ());
  // Incomplete: only one group.
  TEST(!MatchLatLonDegree("37 00 27.73 N", lat, lon), ());
}

UNIT_TEST(LatLon_Match_False)
{
  double lat, lon;
  TEST(!MatchLatLonDegree("2 1st", lat, lon), ());
}

}  // namespace latlon_match_test
