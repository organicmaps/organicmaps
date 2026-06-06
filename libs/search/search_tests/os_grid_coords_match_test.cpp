#include "testing/testing.hpp"

#include "search/os_grid_coords_match.hpp"

#include "geometry/latlon.hpp"

namespace os_grid_coords_match_test
{
using search::MatchOSGridCoords;

UNIT_TEST(MatchOSGridCoords)
{
  TEST(!MatchOSGridCoords("  "), ());
  TEST(!MatchOSGridCoords("London"), ());            // Letters out of the GB grid.
  TEST(!MatchOSGridCoords("40.7128 -74.0060"), ());  // Decimal coordinates, not a grid reference.

  // Ambiguous short references that collide with UK postcode districts / ordinary queries must
  // not be matched, otherwise a coordinate result would suppress the normal search for them.
  TEST(!MatchOSGridCoords("SW12"), ());  // Postcode district, not a 10 km grid square.
  TEST(!MatchOSGridCoords("SE10"), ());
  TEST(!MatchOSGridCoords("HP13"), ());
  TEST(!MatchOSGridCoords("NO 10"), ());    // Single 2-digit group.
  TEST(!MatchOSGridCoords("NY 1000"), ());  // Single 4-digit group: needs "NY 10 00" or 6+ digits.

  // Spacing and case variants all resolve to the same point (Snowdon summit).
  auto const a = MatchOSGridCoords("SH 60989 54379");
  auto const b = MatchOSGridCoords("SH6098954379");
  auto const c = MatchOSGridCoords("sh 60989 54379");
  TEST(a && b && c, ());
  TEST(ms::AlmostEqualAbs(*a, ms::LatLon(53.0684972, -4.0762306), 1e-4), (*a));
  TEST(ms::AlmostEqualAbs(*a, *b, 1e-9), ());
  TEST(ms::AlmostEqualAbs(*a, *c, 1e-9), ());

  // A 6-figure reference resolves within one 100 m cell of the Ben Nevis summit.
  TEST(ms::AlmostEqualAbs(*MatchOSGridCoords("NN 166 712"), ms::LatLon(56.79685, -5.003508), 2e-3), ());
}
}  // namespace os_grid_coords_match_test
