#include "testing/testing.hpp"

#include "drape_frontend/perspective_tile_coverage.hpp"
#include "drape_frontend/requested_tiles.hpp"

#include "base/math.hpp"
#include "geometry/mercator.hpp"

#include <cmath>

namespace
{
ScreenBase MakeScreen(int zoom, double tilt, double bearing, double longitude, double anchorY)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 1280, 480);
  double const scale = 360.0 / (std::exp2(zoom - std::log2(512.0 / 256.0 / 1.5)) * 512.0);
  m2::PointD const position(longitude, mercator::LatToY(55.75));
  screen.SetFromParams(position, math::DegToRad(bearing), scale);
  if (tilt > 0)
    screen.ApplyPerspective(math::DegToRad(tilt), math::DegToRad(tilt), screen.GetAngleFOV());
  screen.MatchGandP3d(position, {640, 480 * anchorY});
  return screen;
}
}  // namespace

UNIT_TEST(PerspectiveTiles_CoverGroundAndKeepDetailNearAnchor)
{
  for (int zoom : {10, 17, 18, 20})
    for (double tilt : {0.0, 30.0, 45.0, 55.0})
      for (double bearing : {0.0, 45.0, 90.0})
        for (double longitude : {37.6, 179.999, -179.999})
          for (double anchorY : {0.1, 0.75})
          {
            auto const screen = MakeScreen(zoom, tilt, bearing, longitude, anchorY);
            double const extension = 75.0 * 1.5 * screen.GetScale();
            auto const tiles = df::SelectPerspectiveTiles(screen, zoom, extension, {0.5, anchorY}, {});
            TEST(!tiles.empty(), (zoom, tilt, bearing));
            for (int y = 0; y <= 8; ++y)
              for (int x = 0; x <= 16; ++x)
              {
                auto const point = screen.PtoG(screen.P3dtoP({1280.0 * x / 16, 480.0 * y / 8}));
                bool covered = false;
                for (auto const & key : tiles)
                  covered |= key.GetGlobalRect().IsPointInside(point);
                TEST(covered, (zoom, tilt, bearing, longitude, x, y));
              }
            auto const car = screen.PtoG(screen.P3dtoP({640, 480 * anchorY}));
            bool detailed = false;
            for (auto const & key : tiles)
              detailed |= key.m_zoomLevel == zoom && key.GetGlobalRect().IsPointInside(car);
            TEST(detailed, (zoom, tilt, anchorY));
            TEST(tiles == df::SelectPerspectiveTiles(screen, zoom, extension, {0.5, anchorY}, tiles), ());
          }
}

UNIT_TEST(PerspectiveTiles_ReduceHighTiltCoverageWithoutOverlappingCells)
{
  auto const screen = MakeScreen(18, 55, 45, 37.6, 0.85);
  double const extension = 75 * 1.5 * screen.GetScale();
  auto rect = screen.ClipRect();
  rect.Inflate(extension, extension);
  auto const legacy = df::CalcTilesCoverage(rect, df::ClipTileZoomByMaxDataZoom(18), nullptr);
  auto const tiles = df::SelectPerspectiveTiles(screen, 18, extension, {0.5, 0.85}, {});
  TEST_LESS(tiles.size() * 2, legacy.GetTilesCount(), (tiles.size(), legacy.GetTilesCount()));
  bool coarse = false;
  for (auto a = tiles.begin(); a != tiles.end(); ++a)
  {
    coarse |= a->m_zoomLevel < 18;
    TEST_EQUAL(a->GetRenderZoom(), 18, ());
    df::TileKey const copy(*a, 7, 9);
    TEST(copy == *a, ());
    TEST_EQUAL(copy.GetCanonicalTileKey().GetRenderZoom(), 18, ());
    for (auto b = std::next(a); b != tiles.end(); ++b)
    {
      auto intersection = a->GetGlobalRect();
      if (intersection.Intersect(b->GetGlobalRect()))
        TEST(intersection.SizeX() < 1e-10 || intersection.SizeY() < 1e-10, (*a, *b));
    }
  }
  TEST(coarse, ());
}

UNIT_TEST(PerspectiveTiles_ReenteredCellInvalidatesCoalescedCoverage)
{
  df::RequestedTiles requests(true);
  ScreenBase screen;
  bool buildings, force, marks;
  df::TTilesCollection const first{df::TileKey(0, 0, 18)};
  df::TTilesCollection const second{df::TileKey(1, 0, 18)};
  requests.Set(screen, false, false, false, df::TTilesCollection(first));
  TEST(requests.Get(screen, buildings, force, marks) == first, ());
  TEST(!force, ());
  requests.Set(screen, false, false, false, df::TTilesCollection(second));
  requests.Set(screen, false, false, false, df::TTilesCollection(first));
  TEST(requests.Get(screen, buildings, force, marks) == first, ());
  TEST(force, ());
  TEST(requests.CheckTileKey(*first.begin()), ());
  TEST(!requests.CheckTileKey(*second.begin()), ());
}
