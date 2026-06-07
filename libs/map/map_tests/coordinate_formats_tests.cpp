#include "testing/testing.hpp"

#include "map/place_page_info.hpp"

#include "geometry/latlon.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace
{
using place_page::CoordinatesFormat;
using place_page::FormatCoordinateDisplay;
using place_page::FormatCoordinateValue;

// Display string for a format, or a sentinel when the format is unavailable at this location.
// (std::optional has no DebugPrint, so we compare plain strings.)
std::string Display(CoordinatesFormat f, ms::LatLon ll, std::string_view region)
{
  return FormatCoordinateDisplay(f, ll, region).value_or("<unavailable>");
}

std::string Value(CoordinatesFormat f, ms::LatLon ll, std::string_view region)
{
  return FormatCoordinateValue(f, ll, region).value_or("<unavailable>");
}

bool Available(CoordinatesFormat f, ms::LatLon ll, std::string_view region)
{
  return FormatCoordinateValue(f, ll, region).has_value();
}

// Reference points with their mwm region id and whether the OS Grid applies there.
ms::LatLon const kLondon{51.5074, -0.1278};
ms::LatLon const kEdinburgh{55.9533, -3.1883};
ms::LatLon const kBelfast{54.5973, -5.9301};
ms::LatLon const kDublin{53.3498, -6.2603};
ms::LatLon const kParis{48.8566, 2.3522};
ms::LatLon const kNorthPole{85.0, 10.0};  // Beyond the UTM/MGRS valid latitude (|lat| > 84).
}  // namespace

// AllCoordinateFormats() is the single source of cycle/display order: the stable ids 0..N ascending.
UNIT_TEST(CoordinateFormats_Order)
{
  auto const & all = place_page::AllCoordinateFormats();
  TEST_EQUAL(all.size(), 9, ());
  TEST_EQUAL(static_cast<int>(all[0]), 0, ());  // LatLonDMS
  TEST_EQUAL(static_cast<int>(all[1]), 1, ());  // LatLonDecimal
  TEST_EQUAL(static_cast<int>(all[2]), 2, ());  // OLCFull
  TEST_EQUAL(static_cast<int>(all[3]), 3, ());  // OSMLink
  TEST_EQUAL(static_cast<int>(all[4]), 4, ());  // UTM
  TEST_EQUAL(static_cast<int>(all[5]), 5, ());  // MGRS
  TEST_EQUAL(static_cast<int>(all[6]), 6, ());  // OSGB
  TEST_EQUAL(static_cast<int>(all[7]), 7, ());  // IrishGrid
  TEST_EQUAL(static_cast<int>(all[8]), 8, ());  // ITM
}

// Availability gating: the decimal formats are everywhere; OSGB only in its region; UTM/MGRS not at the poles.
UNIT_TEST(CoordinateFormats_Availability)
{
  std::string_view const noRegion;

  // The four region-independent formats are always available, even at the pole.
  for (auto const f : {CoordinatesFormat::LatLonDMS, CoordinatesFormat::LatLonDecimal, CoordinatesFormat::OLCFull,
                       CoordinatesFormat::OSMLink})
  {
    TEST(Available(f, kLondon, "UK_England_Greater London"), (static_cast<int>(f)));
    TEST(Available(f, kNorthPole, noRegion), (static_cast<int>(f)));
  }

  // OSGB is the official reference only in Great Britain and the Isle of Man.
  TEST(Available(CoordinatesFormat::OSGB, kLondon, "UK_England_Greater London"), ());
  TEST(Available(CoordinatesFormat::OSGB, kEdinburgh, "UK_Scotland_North"), ());
  // Northern Ireland and the Republic of Ireland fall inside the projection rectangle but use the
  // Irish Grid, so the region gate (not an empty projection) must suppress OSGB there.
  TEST(!Available(CoordinatesFormat::OSGB, kBelfast, "UK_Northern Ireland"), ());
  TEST(!Available(CoordinatesFormat::OSGB, kDublin, "Ireland_Leinster"), ());
  TEST(!Available(CoordinatesFormat::OSGB, kParis, "France"), ());

  // UTM/MGRS exist everywhere except beyond their valid latitudes (no user-visible "N/A" any more).
  TEST(Available(CoordinatesFormat::UTM, kLondon, "UK_England_Greater London"), ());
  TEST(Available(CoordinatesFormat::MGRS, kLondon, "UK_England_Greater London"), ());
  TEST(!Available(CoordinatesFormat::UTM, kNorthPole, noRegion), ());
  TEST(!Available(CoordinatesFormat::MGRS, kNorthPole, noRegion), ());

  // The Irish systems (Irish Grid + ITM) are the official reference in Northern Ireland and the
  // Republic of Ireland, and only there - mutually exclusive with OSGB by region.
  for (auto const f : {CoordinatesFormat::IrishGrid, CoordinatesFormat::ITM})
  {
    TEST(Available(f, kBelfast, "UK_Northern Ireland"), (static_cast<int>(f)));
    TEST(Available(f, kDublin, "Ireland_Leinster"), (static_cast<int>(f)));
    TEST(!Available(f, kLondon, "UK_England_Greater London"), (static_cast<int>(f)));
    TEST(!Available(f, kParis, "France"), (static_cast<int>(f)));
    TEST(!Available(f, kNorthPole, noRegion), (static_cast<int>(f)));
  }
}

// Structural invariants of the display string, including the deliberate behaviour deltas D1-D3.
UNIT_TEST(CoordinateFormats_Structure)
{
  std::string_view const region = "UK_England_Greater London";

  // D1: the DMS string has no comma between lat and lon (the single source dropped it).
  TEST_EQUAL(Display(CoordinatesFormat::LatLonDMS, kLondon, region).find(','), std::string::npos, ());
  // The decimal string keeps the comma separator.
  TEST_NOT_EQUAL(Display(CoordinatesFormat::LatLonDecimal, kLondon, region).find(", "), std::string::npos, ());

  // Labelled formats compose as "<label>: <value>"; the bare value carries no label.
  for (auto const & [f, label] :
       {std::pair{CoordinatesFormat::UTM, "UTM: "}, std::pair{CoordinatesFormat::MGRS, "MGRS: "},
        std::pair{CoordinatesFormat::OSGB, "OSGB: "}})
  {
    auto const value = FormatCoordinateValue(f, kLondon, region);
    auto const display = FormatCoordinateDisplay(f, kLondon, region);
    TEST(value && display, (static_cast<int>(f)));
    TEST_EQUAL(*display, label + *value, ());
  }

  // Unlabelled formats: display == bare value.
  for (auto const f : {CoordinatesFormat::LatLonDMS, CoordinatesFormat::LatLonDecimal, CoordinatesFormat::OLCFull,
                       CoordinatesFormat::OSMLink})
    TEST_EQUAL(Value(f, kLondon, region), Display(f, kLondon, region), ());

  // The Irish formats are labelled too, in their own region (Belfast / Northern Ireland).
  std::string_view const ni = "UK_Northern Ireland";
  for (auto const & [f, label] :
       {std::pair{CoordinatesFormat::IrishGrid, "Irish Grid: "}, std::pair{CoordinatesFormat::ITM, "ITM: "}})
  {
    auto const value = FormatCoordinateValue(f, kBelfast, ni);
    auto const display = FormatCoordinateDisplay(f, kBelfast, ni);
    TEST(value && display, (static_cast<int>(f)));
    TEST_EQUAL(*display, label + *value, ());
  }
}

// Golden display strings - the regression net. Pinned to the current formatter output; the DMS
// string uses D1 (space, not comma, between lat and lon).
UNIT_TEST(CoordinateFormats_GoldenStrings)
{
  std::string_view const london = "UK_England_Greater London";
  TEST_EQUAL(Display(CoordinatesFormat::LatLonDMS, kLondon, london), "51°30′26.64″N 00°07′40.08″W", ());
  TEST_EQUAL(Display(CoordinatesFormat::LatLonDecimal, kLondon, london), "51.5074, -0.1278", ());
  TEST_EQUAL(Display(CoordinatesFormat::OLCFull, kLondon, london), "9C3XGV4C+XV", ());
  TEST_EQUAL(Display(CoordinatesFormat::OSMLink, kLondon, london), "https://osm.org/go/euu4gZqx-?m", ());
  TEST_EQUAL(Display(CoordinatesFormat::UTM, kLondon, london), "UTM: 30U 699316 5710164", ());
  TEST_EQUAL(Display(CoordinatesFormat::MGRS, kLondon, london), "MGRS: 30U XC 99316 10163", ());
  TEST_EQUAL(Display(CoordinatesFormat::OSGB, kLondon, london), "OSGB: TQ 3002 8038", ());

  std::string_view const edinburgh = "UK_Scotland_North";
  TEST_EQUAL(Display(CoordinatesFormat::LatLonDMS, kEdinburgh, edinburgh), "55°57′11.88″N 03°11′17.88″W", ());
  TEST_EQUAL(Display(CoordinatesFormat::UTM, kEdinburgh, edinburgh), "UTM: 30U 488242 6200898", ());
  TEST_EQUAL(Display(CoordinatesFormat::OSGB, kEdinburgh, edinburgh), "OSGB: NT 2589 7400", ());

  // D2: at the pole the region-independent formats still work; UTM/MGRS/OSGB drop out (no "N/A").
  TEST_EQUAL(Display(CoordinatesFormat::LatLonDecimal, kNorthPole, ""), "85, 10", ());
  TEST_EQUAL(Display(CoordinatesFormat::UTM, kNorthPole, ""), "<unavailable>", ());
}
