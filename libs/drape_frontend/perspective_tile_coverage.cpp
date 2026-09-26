#include "drape_frontend/perspective_tile_coverage.hpp"

#include "indexer/scales.hpp"

#include "base/buffer_vector.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <array>

namespace df
{
namespace
{
using Polygon = buffer_vector<m2::PointD, 8>;

// Clip the ground viewport to a cell before projecting it. Projecting arbitrary cell corners
// could cross the perspective horizon and produce an inverted screen-space bounding box.
Polygon Clip(Polygon polygon, m2::RectD const & rect)
{
  for (int edge = 0; edge < 4 && !polygon.empty(); ++edge)
  {
    auto distance = [&](m2::PointD const & p)
    {
      switch (edge)
      {
      case 0: return p.x - rect.minX();
      case 1: return rect.maxX() - p.x;
      case 2: return p.y - rect.minY();
      default: return rect.maxY() - p.y;
      }
    };
    Polygon clipped;
    auto a = polygon.back();
    double da = distance(a);
    for (auto const & b : polygon)
    {
      double const db = distance(b);
      if ((da >= 0.0) != (db >= 0.0))
        clipped.push_back(a + (b - a) * (da / (da - db)));
      if (db >= 0.0)
        clipped.push_back(b);
      a = b;
      da = db;
    }
    polygon = std::move(clipped);
  }
  return polygon;
}
}  // namespace

TTilesCollection SelectPerspectiveTiles(ScreenBase const & screen, int renderZoom, double extension,
                                        m2::PointD const & anchor, TTilesCollection const & previous)
{
  auto const & viewport = screen.PixelRectIn3d();
  Polygon ground{{viewport.minX(), viewport.minY()},
                 {viewport.maxX(), viewport.minY()},
                 {viewport.maxX(), viewport.maxY()},
                 {viewport.minX(), viewport.maxY()}};
  m2::RectD bounds;
  for (auto & p : ground)
  {
    p = screen.PtoG(screen.P3dtoP(p));
    bounds.Add(p);
  }
  bounds.Inflate(extension, extension);

  int const finest = ClipTileZoomByMaxDataZoom(renderZoom);
  bool const useLod = screen.GetRotationAngle() >= math::DegToRad(35.0) && anchor.y > 0.15;
  int const coarsest = useLod ? std::min(finest, std::max(scales::GetUpperWorldScale() + 1, finest - 3)) : finest;
  // A custom anchor near the top moves the detailed band upwards as well.
  double const nearBand = std::clamp(anchor.y - 0.15, 0.0, 0.55);
  TTilesCollection tiles;
  auto visit = [&](auto && self, int x, int y, int zoom) -> void
  {
    TileKey key(x, y, static_cast<uint8_t>(zoom == finest ? renderZoom : zoom));
    if (zoom != finest)
      key.m_renderZoom = static_cast<uint8_t>(renderZoom);
    auto rect = key.GetGlobalRect();
    rect.Inflate(extension, extension);
    auto const clipped = Clip(ground, rect);
    if (clipped.empty())
      return;

    if (zoom < finest)
    {
      double bottom = viewport.minY();
      for (auto const & p : clipped)
        bottom = std::max(bottom, screen.PtoP3d(screen.GtoP(p)).y);
      int const delta = finest - zoom;
      double threshold = nearBand * (delta >= 3 ? 0.27 : delta == 2 ? 0.55 : 1.0);
      // Keep the existing LOD around a boundary instead of re-reading tiles on every small GPS change.
      threshold += previous.contains(key) ? 0.025 : -0.025;
      if (bottom >= viewport.minY() + std::max(0.0, threshold) * viewport.SizeY())
      {
        for (int dy = 0; dy < 2; ++dy)
          for (int dx = 0; dx < 2; ++dx)
            self(self, x * 2 + dx, y * 2 + dy, zoom + 1);
        return;
      }
    }
    tiles.insert(key);
  };
  CalcTilesCoverage(bounds, coarsest, [&](int x, int y) { visit(visit, x, y, coarsest); });
  return tiles;
}
}  // namespace df
