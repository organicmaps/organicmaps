#include "testing/testing.hpp"

#include "platform/os_grid_utils.hpp"

#include "geometry/latlon.hpp"

using namespace os_grid_utils;

UNIT_TEST(OSGrid_Format_GoldenValues)
{
  // WGS84 coordinates and OS grid references taken from Wikipedia infoboxes (OSTN15-derived).
  // A 100 m (6-figure) reference is well within the ~5 m accuracy of the Helmert datum transform.
  TEST_EQUAL(FormatOSGrid(56.79685, -5.003508, 3), "NN 166 712", ());     // Ben Nevis.
  TEST_EQUAL(FormatOSGrid(54.45424, -3.21160, 3), "NY 215 072", ());      // Scafell Pike.
  TEST_EQUAL(FormatOSGrid(53.0684972, -4.0762306, 3), "SH 609 543", ());  // Snowdon.

  // Default precision is 8-figure (10 m).
  TEST_EQUAL(FormatOSGrid(53.0684972, -4.0762306), "SH 6098 5437", ());
}

UNIT_TEST(OSGrid_Format_OutsideGreatBritain)
{
  TEST(FormatOSGrid(48.8566, 2.3522).empty(), ());    // Paris (south of GB).
  TEST(FormatOSGrid(40.7128, -74.0060).empty(), ());  // New York (west of GB).
  TEST(FormatOSGrid(90.0, 0.0).empty(), ());          // North Pole.
  TEST(FormatOSGrid(55.0, -12.0).empty(), ());        // North Atlantic, west of the grid.
  TEST(FormatOSGrid(51.0, 2.5).empty(), ());          // Eastern Channel: passes the bbox, fails the E/N grid.
}

UNIT_TEST(OSGrid_Parse_Basic)
{
  auto const ll = OSGridToLatLon("NN 166 712");
  TEST(ll, ());
  // A 6-figure reference denotes the SW corner of a 100 m cell (same convention as MGRS),
  // so it lies just south-west of the Ben Nevis summit, within one cell (~0.002 deg here).
  TEST(ll->m_lat < 56.79685 && ll->m_lat > 56.79685 - 2e-3, (*ll));
  TEST(ll->m_lon < -5.003508 && ll->m_lon > -5.003508 - 2e-3, (*ll));
}

UNIT_TEST(OSGrid_Parse_Variants)
{
  auto const a = OSGridToLatLon("SH6098954379");
  auto const b = OSGridToLatLon("SH 60989 54379");
  auto const c = OSGridToLatLon("sh 60989 54379");
  TEST(a && b && c, ());
  TEST(ms::AlmostEqualAbs(*a, *b, 1e-9), ());
  TEST(ms::AlmostEqualAbs(*a, *c, 1e-9), ());
  TEST(ms::AlmostEqualAbs(*a, ms::LatLon(53.0684972, -4.0762306), 1e-4), (*a));
}

UNIT_TEST(OSGrid_Parse_Invalid)
{
  TEST(!OSGridToLatLon(""), ());
  TEST(!OSGridToLatLon("S"), ());
  TEST(!OSGridToLatLon("II 123 456"), ());  // 'I' is not a valid grid letter.
  TEST(!OSGridToLatLon("SW 123 45"), ());   // Mismatched easting/northing lengths.
  TEST(!OSGridToLatLon("SW 12X 456"), ());  // Non-digit.
  TEST(!OSGridToLatLon("ZZ 123 456"), ());  // Letters outside the GB grid.
  TEST(!OSGridToLatLon("12 345 678"), ());  // Not a letter-prefixed reference.
}

UNIT_TEST(OSGrid_IsOSGridRegion)
{
  // Great Britain and the Isle of Man use the British National Grid.
  TEST(IsOSGridRegion("UK_England_Greater London"), ());
  TEST(IsOSGridRegion("UK_Scotland_North"), ());
  TEST(IsOSGridRegion("UK_Wales"), ());
  TEST(IsOSGridRegion("Isle of Man"), ());

  // Northern Ireland and the Republic of Ireland use the Irish Grid, not the BNG.
  TEST(!IsOSGridRegion("UK_Northern Ireland"), ());
  TEST(!IsOSGridRegion("Ireland"), ());

  TEST(!IsOSGridRegion("France"), ());
  TEST(!IsOSGridRegion(""), ());
}

UNIT_TEST(OSGrid_RoundTrip)
{
  ms::LatLon const points[] = {
      {50.0664, -5.7147},  // Land's End.
      {51.5074, -0.1278},  // London.
      {55.9533, -3.1883},  // Edinburgh.
      {58.6373, -3.0689},  // John o' Groats.
      {60.1538, -1.1450},  // Lerwick, Shetland.
      {51.4779, -0.0015},  // Greenwich.
  };

  for (auto const & ll : points)
  {
    auto const grid = FormatOSGrid(ll.m_lat, ll.m_lon, 5 /* 1 m */);
    TEST(!grid.empty(), (ll));
    auto const back = OSGridToLatLon(grid);
    TEST(back, (grid));
    TEST(ms::AlmostEqualAbs(*back, ll, 3e-5), (ll, *back, grid));  // ~3 m.
  }
}
