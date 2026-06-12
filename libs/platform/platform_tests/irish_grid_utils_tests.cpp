#include "testing/testing.hpp"

#include "platform/irish_grid_utils.hpp"

#include "geometry/latlon.hpp"

using namespace irish_grid_utils;

namespace
{
// WGS84 reference points on the island of Ireland.
ms::LatLon const kDublin{53.34976, -6.26030};   // The Spire, O'Connell Street.
ms::LatLon const kBelfast{54.59653, -5.93012};  // City Hall.
ms::LatLon const kGalway{53.27194, -9.04889};   // Eyre Square.
ms::LatLon const kCork{51.89797, -8.47565};     // Cork city.
}  // namespace

// Golden values, cross-validated against published references: Belfast City Hall IG ~ J 338 740 and
// ITM ~ 733750 874000; Galway ~ M 300 251; Dublin Spire ITM ~ 715830 734697. Pinned to the formatter
// output (8-figure / 10 m Irish Grid, 1 m ITM).
UNIT_TEST(IrishGrid_GoldenValues)
{
  TEST_EQUAL(FormatIrishGrid(kDublin.m_lat, kDublin.m_lon), "O 1592 3468", ());
  TEST_EQUAL(FormatITM(kDublin.m_lat, kDublin.m_lon), "715827 734693", ());
  TEST_EQUAL(FormatIrishGrid(kBelfast.m_lat, kBelfast.m_lon), "J 3385 7402", ());
  TEST_EQUAL(FormatITM(kBelfast.m_lat, kBelfast.m_lon), "733752 873998", ());
  TEST_EQUAL(FormatIrishGrid(kGalway.m_lat, kGalway.m_lon), "M 3009 2512", ());
  TEST_EQUAL(FormatITM(kGalway.m_lat, kGalway.m_lon), "530037 725136", ());
  TEST_EQUAL(FormatIrishGrid(kCork.m_lat, kCork.m_lon), "W 6733 7182", ());
  TEST_EQUAL(FormatITM(kCork.m_lat, kCork.m_lon), "567265 571864", ());
}

UNIT_TEST(IrishGrid_OutsideIreland)
{
  TEST(FormatIrishGrid(51.5074, -0.1278).empty(), ());  // London.
  TEST(FormatIrishGrid(48.8566, 2.3522).empty(), ());   // Paris.
  TEST(FormatITM(51.5074, -0.1278).empty(), ());        // London.
  TEST(FormatITM(40.7128, -74.0060).empty(), ());       // New York.
}

UNIT_TEST(IrishGrid_RoundTrip)
{
  ms::LatLon const points[] = {kDublin, kBelfast, kGalway, kCork, {52.6680, -8.6305} /* Limerick */};
  for (auto const & ll : points)
  {
    auto const ig = FormatIrishGrid(ll.m_lat, ll.m_lon, 5 /* 1 m */);
    TEST(!ig.empty(), (ll));
    auto const igBack = IrishGridToLatLon(ig);
    TEST(igBack, (ig));
    TEST(ms::AlmostEqualAbs(*igBack, ll, 3e-5), (ll, *igBack, ig));  // ~3 m (datum + 1 m grid).

    auto const itm = FormatITM(ll.m_lat, ll.m_lon);
    TEST(!itm.empty(), (ll));
    auto const itmBack = ITMToLatLon(itm);
    TEST(itmBack, (itm));
    TEST(ms::AlmostEqualAbs(*itmBack, ll, 2e-5), (ll, *itmBack, itm));  // ~1-2 m (rounding only).
  }
}

UNIT_TEST(IrishGrid_Parse_Variants)
{
  auto const a = IrishGridToLatLon("O 1516 3447");
  auto const b = IrishGridToLatLon("O15163447");
  auto const c = IrishGridToLatLon("o 1516 3447");
  TEST(a && b && c, ());
  TEST(ms::AlmostEqualAbs(*a, *b, 1e-9), ());
  TEST(ms::AlmostEqualAbs(*a, *c, 1e-9), ());
}

UNIT_TEST(IrishGrid_Parse_Invalid)
{
  TEST(!IrishGridToLatLon(""), ());
  TEST(!IrishGridToLatLon("O"), ());
  TEST(!IrishGridToLatLon("I 123 456"), ());   // 'I' is not a valid grid letter.
  TEST(!IrishGridToLatLon("O 12 345"), ());    // Mismatched easting/northing lengths.
  TEST(!IrishGridToLatLon("O 12X 456"), ());   // Non-digit.
  TEST(!IrishGridToLatLon("12 345 678"), ());  // Not a letter-prefixed reference.

  TEST(!ITMToLatLon(""), ());
  TEST(!ITMToLatLon("715830"), ());            // Single number.
  TEST(!ITMToLatLon("715830 734697 1"), ());   // Three numbers.
  TEST(!ITMToLatLon("123456789 734697"), ());  // Over-long group: rejected, never overflows.
  TEST(!ITMToLatLon("99999999999999 1"), ());  // Far over the digit bound.
}

// The parsers do not gate on geography (the search processor does that, by region): a well-formed
// in-grid reference resolves to a point even when it lies off the island, in the sea or W Scotland.
UNIT_TEST(IrishGrid_Parse_OffIslandStillParses)
{
  TEST(IrishGridToLatLon("D 5200 6746"), ());  // Decodes across the North Channel into Scotland (Kintyre).
  TEST(ITMToLatLon("999999 999999"), ());      // Well-formed 6+6 pair far off the island.
}

UNIT_TEST(IrishGrid_IsIrishGridRegion)
{
  TEST(IsIrishGridRegion("UK_Northern Ireland"), ());
  // The Republic's place pages report the province leaf mwm, not the bare "Ireland" group node.
  TEST(IsIrishGridRegion("Ireland_Leinster"), ());  // Dublin.
  TEST(IsIrishGridRegion("Ireland_Munster"), ());   // Cork.
  TEST(IsIrishGridRegion("Ireland_Connacht"), ());  // Galway.
  TEST(IsIrishGridRegion("Ireland_Northern Counties"), ());

  TEST(!IsIrishGridRegion("UK_England_Greater London"), ());
  TEST(!IsIrishGridRegion("UK_Scotland_North"), ());
  TEST(!IsIrishGridRegion("Isle of Man"), ());
  TEST(!IsIrishGridRegion("New Ireland"), ());  // A different place; must not match the "Ireland" prefix.
  TEST(!IsIrishGridRegion("France"), ());
  TEST(!IsIrishGridRegion(""), ());
}
