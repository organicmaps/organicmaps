#include "testing/testing.hpp"

#include "search/irish_grid_coords_match.hpp"

#include "platform/irish_grid_utils.hpp"

#include "geometry/latlon.hpp"

namespace irish_grid_coords_match_test
{
using search::MatchIrishGridCoords;
using search::MatchITMCoords;

ms::LatLon const kDublin{53.34976, -6.26030};
ms::LatLon const kBelfast{54.59653, -5.93012};

UNIT_TEST(MatchIrishGridCoords_Positive)
{
  // Round-trips of the formatter output (8-figure) - the copy/paste path from the place page.
  for (auto const & ll : {kDublin, kBelfast})
  {
    auto const ref = irish_grid_utils::FormatIrishGrid(ll.m_lat, ll.m_lon);
    auto const m = MatchIrishGridCoords(ref);
    TEST(m, (ref));
    TEST(ms::AlmostEqualAbs(*m, ll, 1e-3), (ref, *m, ll));
  }

  // 6-figure references (the common hand-entered precision) and spacing/case variants all resolve.
  TEST(MatchIrishGridCoords("O 152 345"), ());
  TEST(MatchIrishGridCoords("O152345"), ());
  TEST(MatchIrishGridCoords("o 152 345"), ());

  // A "letter + 6 digits" string that is on the island is intentionally a (weak) match: the processor
  // shows it as a coordinate AND still runs the normal search, so this is safe even if it was not meant
  // as a coordinate. Validity here just means "in range".
  TEST(MatchIrishGridCoords("M 123 456"), ());
}

UNIT_TEST(MatchIrishGridCoords_NoCoordinate)
{
  TEST(!MatchIrishGridCoords(""), ());
  TEST(!MatchIrishGridCoords("   "), ());

  // Road numbers, Dublin districts and brands: a single letter with too few digits - no coordinate at all.
  for (char const * s : {"N4", "M50", "M1", "R110", "N11", "D4", "D6", "D6W", "O2"})
    TEST(!MatchIrishGridCoords(s), (s));

  // Eircodes carry letters in the second group, so a digit-only parse rejects them outright.
  TEST(!MatchIrishGridCoords("D04 X285"), ());
  TEST(!MatchIrishGridCoords("A65 F4E2"), ());

  // Ordinary words, a too-coarse 4-figure ref, an invalid grid letter, a missing leading letter.
  TEST(!MatchIrishGridCoords("Dublin"), ());
  TEST(!MatchIrishGridCoords("O 12 34"), ());      // 2+2 figures: below the 3-digit minimum.
  TEST(!MatchIrishGridCoords("I 1234 5678"), ());  // 'I' is not a valid grid letter.
  TEST(!MatchIrishGridCoords("12 345 678"), ());   // No leading letter.

  // Geographic gating is the processor's job (by region), not the matcher's: a well-formed in-range
  // reference still matches here even if it lies off the island - see the processor integration test.

  // An OS Grid reference (two letters) and an ITM pair (no letter) are handled elsewhere / separately.
  TEST(!MatchIrishGridCoords("SW 7400 4210"), ());
  TEST(!MatchIrishGridCoords("715830 734697"), ());
}

UNIT_TEST(MatchITMCoords_Positive)
{
  for (auto const & ll : {kDublin, kBelfast})
  {
    auto const itm = irish_grid_utils::FormatITM(ll.m_lat, ll.m_lon);
    auto const m = MatchITMCoords(itm);
    TEST(m, (itm));
    TEST(ms::AlmostEqualAbs(*m, ll, 1e-3), (itm, *m, ll));

    // The comma-separated variant resolves the same way.
    std::string comma = itm;
    comma[itm.find(' ')] = ',';
    TEST(MatchITMCoords(comma), (comma));
  }
}

UNIT_TEST(MatchITMCoords_NoCoordinate)
{
  TEST(!MatchITMCoords(""), ());
  TEST(!MatchITMCoords("   "), ());

  // Short pairs, single values, phone numbers, decimals - not an ITM easting/northing pair.
  for (char const * s : {"12 34", "100 200", "1 2", "715830", "087 123 4567", "53.5 -8.0"})
    TEST(!MatchITMCoords(s), (s));

  // Wrong digit counts (ITM is exactly 6 + 6 across the island).
  TEST(!MatchITMCoords("12345 678901"), ());
  TEST(!MatchITMCoords("1234567 890123"), ());
  TEST(!MatchITMCoords("715830 734697 123456"), ());

  // (A correct 6+6 pair that lies off the island still matches here; the processor gates it by region.)

  // An Irish Grid reference and a UTM string are not ITM.
  TEST(!MatchITMCoords("O 152 345"), ());
  TEST(!MatchITMCoords("30U 699316 5710164"), ());
}
}  // namespace irish_grid_coords_match_test
