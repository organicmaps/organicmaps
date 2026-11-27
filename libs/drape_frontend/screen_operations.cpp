#include "screen_operations.hpp"

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/simplification.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <cmath>

namespace df
{

bool IsScaleAllowableIn3d(int scale)
{
  int minScale = scales::GetMinAllowableIn3dScale();
  if (df::VisualParams::Instance().GetVisualScale() <= 1.0)
    --minScale;
  if (GetPlatform().IsTablet())
    ++minScale;
  return scale >= minScale;
}

double CalculateScale(m2::RectD const & pixelRect, m2::RectD const & localRect)
{
  return std::max(localRect.SizeX() / pixelRect.SizeX(), localRect.SizeY() / pixelRect.SizeY());
}

m2::PointD CalculateCenter(double scale, m2::RectD const & pixelRect, m2::PointD const & userPos,
                           m2::PointD const & pixelPos, double azimuth)
{
  m2::PointD formingVector = (pixelRect.Center() - pixelPos) * scale;
  formingVector.y = -formingVector.y;
  formingVector.Rotate(azimuth);
  return userPos + formingVector;
}

m2::PointD CalculateCenter(ScreenBase const & screen, m2::PointD const & userPos, m2::PointD const & pixelPos,
                           double azimuth)
{
  double const scale = screen.GlobalRect().GetLocalRect().SizeX() / screen.PixelRect().SizeX();
  return CalculateCenter(scale, screen.PixelRect(), userPos, pixelPos, azimuth);
}

bool CheckMinScale(ScreenBase const & screen)
{
  m2::RectD const & r = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();

  // X must fit one world copy (no duplicates across the antimeridian); Y must fit between the poles.
  return r.SizeX() <= worldR.SizeX() && r.SizeY() <= worldR.SizeY();
}

bool CheckMaxScale(ScreenBase const & screen)
{
  VisualParams const & p = VisualParams::Instance();
  return CheckMaxScale(screen, p.GetTileSize(), p.GetVisualScale());
}

bool CheckMaxScale(ScreenBase const & screen, uint32_t tileSize, double visualScale)
{
  return (df::GetDrawTileScale(screen, tileSize, visualScale) <= scales::GetUpperStyleScale());
}

bool CheckBorders(ScreenBase const & screen)
{
  m2::RectD const & r = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();

  // Y must stay between the poles. X is intentionally unchecked — the world wraps at the antimeridian.
  return r.minY() >= worldR.minY() && r.maxY() <= worldR.maxY();
}

bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  m2::RectD const & clipRect = screen.ClipRect();
  return boundRect.SizeX() >= clipRect.SizeX() && boundRect.SizeY() >= clipRect.SizeY();
}

void ShrinkInto(ScreenBase & screen, m2::RectD const & boundRect)
{
  bool changed = false;

  m2::RectD clipRect = screen.ClipRect();
  if (clipRect.minY() < boundRect.minY())
  {
    changed = true;
    clipRect.Offset(0, boundRect.minY() - clipRect.minY());
  }
  if (clipRect.maxY() > boundRect.maxY())
  {
    changed = true;
    clipRect.Offset(0, boundRect.maxY() - clipRect.maxY());
  }

  if (changed)
    screen.SetOrg(clipRect.Center());
}

void ScaleInto(ScreenBase & screen, m2::RectD const & boundRect)
{
  double scale = 1;
  m2::RectD clipRect = screen.ClipRect();
  bool changed = false;

  auto const DoScale = [&](double k)
  {
    // https://github.com/organicmaps/organicmaps/issues/544
    if (k > 0)
    {
      scale /= k;
      clipRect.Scale(k);
      changed = true;
    }
    else
    {
      // Will break in Debug, log in Release.
      LOG(LERROR, ("Bad scale factor =", k, "Bound rect =", boundRect, "Clip rect =", clipRect));
    }
  };

  if (clipRect.SizeX() > boundRect.SizeX())
    DoScale(boundRect.SizeX() / clipRect.SizeX());

  if (clipRect.minY() < boundRect.minY())
    DoScale((boundRect.minY() - clipRect.Center().y) / (clipRect.minY() - clipRect.Center().y));

  if (clipRect.maxY() > boundRect.maxY())
    DoScale((boundRect.maxY() - clipRect.Center().y) / (clipRect.maxY() - clipRect.Center().y));

  if (changed)
  {
    screen.Scale(scale);
    screen.SetOrg(clipRect.Center());
  }
}

void ShrinkAndScaleInto(ScreenBase & screen, m2::RectD const & boundRect)
{
  m2::RectD globalRect = screen.ClipRect();
  double scale = 1;
  bool changed = false;

  // Constrain X width: viewport must not be wider than one world width (360°).
  if (globalRect.SizeX() > boundRect.SizeX())
  {
    double k = boundRect.SizeX() / globalRect.SizeX();
    scale /= k;
    globalRect.Scale(k);
    changed = true;
  }

  // Constrain Y axis: keep viewport within pole boundaries.
  if (globalRect.minY() < boundRect.minY())
  {
    globalRect.Offset(0, boundRect.minY() - globalRect.minY());
    if (globalRect.maxY() > boundRect.maxY())
    {
      double k = boundRect.SizeY() / globalRect.SizeY();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(0, boundRect.minY() - globalRect.minY());
    }
    changed = true;
  }

  if (globalRect.maxY() > boundRect.maxY())
  {
    globalRect.Offset(0, boundRect.maxY() - globalRect.maxY());
    if (globalRect.minY() < boundRect.minY())
    {
      double k = boundRect.SizeY() / globalRect.SizeY();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(0, boundRect.maxY() - globalRect.maxY());
    }
    changed = true;
  }

  if (changed)
  {
    screen.SetOrg(globalRect.Center());
    screen.Scale(scale);
  }
}

bool ApplyScale(m2::PointD const & pixelScaleCenter, double factor, ScreenBase & screen)
{
  m2::PointD const globalScaleCenter = screen.PtoG(screen.P3dtoP(pixelScaleCenter));

  ScreenBase tmp = screen;
  tmp.Scale(factor);
  tmp.MatchGandP3d(globalScaleCenter, pixelScaleCenter);

  if (!CheckMinScale(tmp))
    return false;

  m2::RectD const & worldR = df::GetWorldRect();

  if (!CheckBorders(tmp))
  {
    if (CanShrinkInto(tmp, worldR))
      ShrinkInto(tmp, worldR);
    else
      return false;
  }

  if (!CheckMaxScale(tmp))
  {
    auto const maxScale = GetScreenScale(scales::GetUpperStyleScale() + 0.42);
    if (maxScale > screen.GetScale() && CheckMaxScale(screen))
      return false;
    tmp.SetScale(maxScale);
  }

  // re-checking the borders, as we might violate them a bit (don't know why).
  if (!CheckBorders(tmp))
    ScaleInto(tmp, worldR);

  screen = tmp;
  return true;
}

void NormalizeScreenOriginX(ScreenBase & screen)
{
  auto org = screen.GetOrg();
  double const kMaxX = 540.0;  // 1.5 world widths
  if (org.x > kMaxX || org.x < -kMaxX)
  {
    double const kRangeX = mercator::Bounds::kRangeX;  // 360.0
    org.x = std::fmod(org.x + kMaxX, kRangeX);
    if (org.x < 0.0)
      org.x += kRangeX;
    org.x -= kMaxX;
    screen.SetOrg(org);
  }
}

m2::PointD AdjustPointForViewport(m2::PointD const & pt, ScreenBase const & screen)
{
  return {mercator::NearestWrapX(pt.x, screen.GetOrg().x), pt.y};
}

m2::PointD GtoPWrap(m2::PointD const & pt, ScreenBase const & screen)
{
  return screen.GtoP(AdjustPointForViewport(pt, screen));
}

m2::PointD PtoGWrap(m2::PointD const & pt, ScreenBase const & screen)
{
  auto mercator = screen.PtoG(pt);
  mercator.x = mercator::WrapX(mercator.x);
  return mercator;
}

namespace
{
std::pair<int, int> VisibleWorldCopies(ScreenBase const & screen, m2::RectD const & bounds)
{
  m2::RectD const & viewport = screen.ClipRect();
  constexpr double kWorldWidth = mercator::Bounds::kRangeX;
  if (viewport.maxY() < bounds.minY() || viewport.minY() > bounds.maxY())
    return {1, 0};
  return {static_cast<int>(std::ceil((viewport.minX() - bounds.maxX()) / kWorldWidth)),
          static_cast<int>(std::floor((viewport.maxX() - bounds.minX()) / kWorldWidth))};
}
void AppendProjectedPath(std::vector<ProjectedPolyline> & paths, double offsetX, std::vector<m2::PointD> pixels)
{
  if (pixels.size() < 2)
    return;

  // Drop tightly spaced samples below half a render pixel before building the accessibility path.
  // Keep the clipped endpoints and retain a distinct point for a small closed loop.
  double constexpr kMinSquaredSpacing = 0.25;
  auto kept = pixels.begin();
  for (auto sample = kept + 1; sample != pixels.end() - 1; ++sample)
    if (kept->SquaredLength(*sample) >= kMinSquaredSpacing)
      *++kept = *sample;
  if (kept == pixels.begin() && *kept == pixels.back())
  {
    auto const distinct =
        std::find_if(kept + 1, pixels.end(), [&kept](auto const & sample) { return sample != *kept; });
    if (distinct == pixels.end())
      return;
    *++kept = *distinct;
  }
  if (*kept != pixels.back())
    *++kept = pixels.back();
  pixels.erase(kept + 1, pixels.end());

  // Bound the simplification search for long, highly irregular tracks.
  size_t constexpr kSimplificationWindow = 64;
  if (pixels.size() > kSimplificationWindow)
  {
    std::vector<m2::PointD> simplified;
    simplified.reserve(pixels.size() / kSimplificationWindow + 1);
    auto const append = [&simplified](m2::PointD const & point)
    {
      if (simplified.empty() || point != simplified.back())
        simplified.push_back(point);
    };
    for (size_t start = 0; start < pixels.size() - 1;)
    {
      size_t const end = std::min(start + kSimplificationWindow, pixels.size() - 1);
      SimplifyDP(pixels.begin() + start, pixels.begin() + end + 1, kMinSquaredSpacing,
                 m2::SquaredDistanceFromSegmentToPoint{}, append);
      start = end;
    }
    pixels = std::move(simplified);
  }
  paths.push_back({offsetX, std::move(pixels)});
}
}  // namespace

bool IsPolylineInViewport(ScreenBase const & screen, m2::RectD const & bounds)
{
  auto const [first, last] = VisibleWorldCopies(screen, bounds);
  return first <= last;
}

std::vector<ProjectedPolyline> ProjectVisiblePolyline(ScreenBase const & screen, m2::RectD const & pixelViewport,
                                                      m2::RectD const & bounds, std::vector<m2::PointD> const & line)
{
  std::vector<ProjectedPolyline> result;
  if (line.size() < 2 || !pixelViewport.IsValid())
    return result;

  auto const [first, last] = VisibleWorldCopies(screen, bounds);
  for (int copy = first; copy <= last; ++copy)
  {
    double const offsetX = copy * mercator::Bounds::kRangeX;
    auto worldClip = screen.ClipRect();
    worldClip.Offset(-offsetX, 0.0);

    auto project = [&](std::vector<m2::PointD> const & points)
    {
      std::vector<m2::PointD> projected;
      projected.reserve(points.size());
      m2::RectD projectedBounds;
      for (auto const & point : points)
      {
        auto const pixel = screen.PtoP3d(screen.GtoP({point.x + offsetX, point.y}));
        projectedBounds.Add(pixel);
        projected.push_back(pixel);
      }
      if (!pixelViewport.IsIntersect(projectedBounds))
        return;

      if (pixelViewport.IsRectInside(projectedBounds))
      {
        AppendProjectedPath(result, offsetX, std::move(projected));
        return;
      }

      std::vector<m2::SharedSpline> visiblePaths;
      m2::ClipPathByRect(pixelViewport, projected, visiblePaths);
      for (auto const & path : visiblePaths)
        AppendProjectedPath(result, offsetX, path->GetPath());
    };

    if (worldClip.IsRectInside(bounds))
    {
      project(line);
      continue;
    }

    std::vector<m2::SharedSpline> worldPaths;
    m2::ClipPathByRect(worldClip, line, worldPaths);
    for (auto const & worldPath : worldPaths)
      project(worldPath->GetPath());
  }
  return result;
}

AdjustedScreen::AdjustedScreen(ScreenBase const & screen, m2::PointD const & pivot) : m_screen(screen)
{
  double const wrapX = mercator::NearestWrapX(pivot.x, screen.GetOrg().x);
  m_wrapPivot = {wrapX, pivot.y};
  m_offsetWrapX = pivot.x - wrapX;
}

ScreenBase::Matrix3dT AdjustedScreen::GetShapeModelView() const
{
  return m_screen.GetModelView(m_wrapPivot, kShapeCoordScalar);
}

}  // namespace df
