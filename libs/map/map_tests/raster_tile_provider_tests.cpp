#include "testing/testing.hpp"

#include "map/raster_tile_provider.hpp"

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <cmath>

namespace
{
using RTP = RasterTileProvider;

// Reference: the standard web-mercator (XYZ / "slippy map") tile index for a geographic point.
void StandardWebTile(double lat, double lon, int z, int & x, int & y)
{
  double const n = std::ldexp(1.0, z);  // 2^z
  x = static_cast<int>(std::floor((lon + 180.0) / 360.0 * n));
  double const latRad = lat * math::pi / 180.0;
  y = static_cast<int>(std::floor((1.0 - std::asinh(std::tan(latRad)) / math::pi) / 2.0 * n));
}

// The OM tile that contains the point at the given OM zoomLevel (m_x/m_y in the 2^(zoom-1) grid,
// matching m_zoomLevel — which is exactly the contract ToSourceTile relies on).
df::TileKey OmTileAt(double lat, double lon, int omZoom)
{
  return df::GetTileKeyByPoint(mercator::FromLatLon(lat, lon), omZoom);
}
}  // namespace

// A correctly-built OM tile (grid resolution matches m_zoomLevel) must map to the geographically
// correct web-mercator tile across the whole zoom range, for points in every hemisphere.
UNIT_TEST(RasterTileProvider_ToSourceTile_MatchesWebMercator)
{
  double const pts[][2] = {
      {50.584312, 33.724862},  // the "z18 desert tile" bug report point
      {0.1, 0.1},
      {-33.8688, 151.2093},
      {64.5, -147.0},
      {-54.8, -68.3},
  };
  int const kMaxZoom = 30;  // high enough that no tile is over-zoomed: each maps to a full web tile

  for (auto const & p : pts)
  {
    double const lat = p[0], lon = p[1];
    for (int omZoom = 2; omZoom <= 21; ++omZoom)
    {
      auto const src = RTP::ToSourceTile(OmTileAt(lat, lon, omZoom), 0 /* minZoom */, kMaxZoom);
      TEST(src.m_valid, (lat, lon, omZoom));

      int const z = omZoom - 1;
      int ex = 0, ey = 0;
      StandardWebTile(lat, lon, z, ex, ey);
      TEST_EQUAL(src.m_z, z, (lat, lon, omZoom));
      TEST_EQUAL(src.m_x, ex, (lat, lon, omZoom));
      TEST_EQUAL(src.m_y, ey, (lat, lon, omZoom));
      // Full tile, no sub-rect when not over-zoomed.
      TEST_EQUAL(src.m_rect, m2::RectF(0.0f, 0.0f, 1.0f, 1.0f), (lat, lon, omZoom));
    }
  }
}

// Regression for the "z18 desert tile" bug, with the exact browser-verified web tiles. The bug was
// upstream (the OM coverage was clamped to GetUpperScale()=17, so m_x/m_y lagged m_zoomLevel above
// zoom 17); these values lock in the conversion contract that the fix restores.
UNIT_TEST(RasterTileProvider_ToSourceTile_BugReportCoords)
{
  double const lat = 50.584312, lon = 33.724862;
  struct
  {
    int omZoom, z, x, y;
  } const expected[] = {
      {17, 16, 38907, 22059},
      {18, 17, 77814, 44119},
      {19, 18, 155629, 88238},
  };

  for (auto const & e : expected)
  {
    auto const src = RTP::ToSourceTile(OmTileAt(lat, lon, e.omZoom), 0 /* minZoom */, 30 /* maxZoom */);
    TEST(src.m_valid, (e.omZoom));
    TEST_EQUAL(src.m_z, e.z, (e.omZoom));
    TEST_EQUAL(src.m_x, e.x, (e.omZoom));
    TEST_EQUAL(src.m_y, e.y, (e.omZoom));
  }
}

// Beyond maxZoom the source is the maxZoom ancestor tile plus a sub-rect within it.
UNIT_TEST(RasterTileProvider_ToSourceTile_OverZoom)
{
  int const kMaxZoom = 19;
  double const lat = 50.584312, lon = 33.724862;
  int const omZoom = 22;  // web z = 21, k = 21 - 19 = 2, so a 4x4 grid of children share the ancestor

  auto const src = RTP::ToSourceTile(OmTileAt(lat, lon, omZoom), 0 /* minZoom */, kMaxZoom);
  TEST(src.m_valid, ());
  TEST_EQUAL(src.m_z, kMaxZoom, ());

  int ax = 0, ay = 0;
  StandardWebTile(lat, lon, kMaxZoom, ax, ay);  // the maxZoom ancestor contains the point
  TEST_EQUAL(src.m_x, ax, ());
  TEST_EQUAL(src.m_y, ay, ());

  // The sub-rect is one of the 4x4 = 0.25-sized cells, fully inside the unit square.
  TEST(src.m_rect.minX() >= 0.0f && src.m_rect.maxX() <= 1.0f, (src.m_rect));
  TEST(src.m_rect.minY() >= 0.0f && src.m_rect.maxY() <= 1.0f, (src.m_rect));
  TEST_ALMOST_EQUAL_ABS(src.m_rect.SizeX(), 0.25f, 1e-5f, ());
  TEST_ALMOST_EQUAL_ABS(src.m_rect.SizeY(), 0.25f, 1e-5f, ());
}

// Tiles below the configured minimum source zoom are skipped (reported invalid, never requested).
UNIT_TEST(RasterTileProvider_ToSourceTile_BelowMinZoomInvalid)
{
  // OM zoom 6 -> web z5, below minZoom 7.
  auto const src = RTP::ToSourceTile(OmTileAt(50.584312, 33.724862, 6), 7 /* minZoom */, 19 /* maxZoom */);
  TEST(!src.m_valid, ());
}

// The antimeridian copies (OM x outside the base [-n/2, n/2) range) wrap onto valid [0, n) web tiles.
UNIT_TEST(RasterTileProvider_ToSourceTile_AntimeridianWrap)
{
  int const omZoom = 5;  // web z4, n = 16
  int const n = 1 << (omZoom - 1);
  df::TileKey const base(3, 2, static_cast<uint8_t>(omZoom));
  auto const src0 = RTP::ToSourceTile(base, 0, 30);
  // A whole-world shift east must land on the same web tile.
  df::TileKey const shifted(3 + n, 2, static_cast<uint8_t>(omZoom));
  auto const src1 = RTP::ToSourceTile(shifted, 0, 30);
  TEST(src0.m_valid && src1.m_valid, ());
  TEST_EQUAL(src0.m_x, src1.m_x, ());
  TEST_EQUAL(src0.m_y, src1.m_y, ());
  TEST(src0.m_x >= 0 && src0.m_x < n, (src0.m_x, n));
}
