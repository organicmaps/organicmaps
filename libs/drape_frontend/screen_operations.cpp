#include "screen_operations.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

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

  // Y must not exceed pole boundaries (no scrolling past poles).
  // X is unconstrained — the world wraps horizontally at the antimeridian.
  return r.SizeY() <= worldR.SizeY();
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

  // Viewport Y must fit inside world Y (no scrolling past poles).
  return r.minY() >= worldR.minY() && r.maxY() <= worldR.maxY();
}

bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  m2::RectD const & clipRect = screen.ClipRect();
  // Only check Y — X is unconstrained (world wraps horizontally).
  return boundRect.SizeY() >= clipRect.SizeY();
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

}  // namespace df
