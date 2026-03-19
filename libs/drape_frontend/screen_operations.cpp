#include "screen_operations.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include <algorithm>

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

  return (r.SizeX() <= worldR.SizeX() || r.SizeY() <= worldR.SizeY());
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

  return (r.IsRectInside(worldR) || worldR.IsRectInside(r));
}

bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  m2::RectD clipRect = screen.ClipRect();
  return (boundRect.SizeX() >= clipRect.SizeX()) && (boundRect.SizeY() >= clipRect.SizeY());
}

ScreenBase ShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  ScreenBase res = screen;
  m2::RectD clipRect = res.ClipRect();

  double overscrollWidth = clipRect.SizeX() / 2;
  if (clipRect.maxX() > 180)
  {
    overscrollWidth = clipRect.SizeX() / 2;
  }

  m2::RectD boundWithOverscroll = m2::RectD(boundRect.minX()-overscrollWidth,
    boundRect.minY(), boundRect.maxX() + overscrollWidth, boundRect.maxY());

  if (clipRect.minX() < boundWithOverscroll.minX())
    clipRect.Offset(boundWithOverscroll.minX() - clipRect.minX(), 0);
  if (clipRect.maxX() > boundWithOverscroll.maxX())
    clipRect.Offset(boundWithOverscroll.maxX() - clipRect.maxX(), 0);
  if (clipRect.minY() < boundWithOverscroll.minY())
    clipRect.Offset(0, boundWithOverscroll.minY() - clipRect.minY());
  if (clipRect.maxY() > boundWithOverscroll.maxY())
    clipRect.Offset(0, boundWithOverscroll.maxY() - clipRect.maxY());

  res.SetOrg(clipRect.Center());

  // This assert fails near x = 180 (Philipines).
  // ASSERT ( boundRect.IsRectInside(res.ClipRect()), (clipRect, res.ClipRect()) );
  return res;
}

ScreenBase ScaleInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  ScreenBase res = screen;

  double scale = 1;
  m2::RectD clipRect = res.ClipRect();

  auto const DoScale = [&scale, &clipRect, &boundRect](double k)
  {
    // https://github.com/organicmaps/organicmaps/issues/544
    if (k > 0)
    {
      scale /= k;
      clipRect.Scale(k);
    }
    else
    {
      // Will break in Debug, log in Release.
      LOG(LERROR, ("Bad scale factor =", k, "Bound rect =", boundRect, "Clip rect =", clipRect));
    }
  };

  ASSERT(boundRect.IsPointInside(clipRect.Center()), ("center point should be inside boundRect"));

  if (clipRect.minX() < boundRect.minX())
    DoScale((boundRect.minX() - clipRect.Center().x) / (clipRect.minX() - clipRect.Center().x));

  if (clipRect.maxX() > boundRect.maxX())
    DoScale((boundRect.maxX() - clipRect.Center().x) / (clipRect.maxX() - clipRect.Center().x));

  if (clipRect.minY() < boundRect.minY())
    DoScale((boundRect.minY() - clipRect.Center().y) / (clipRect.minY() - clipRect.Center().y));

  if (clipRect.maxY() > boundRect.maxY())
    DoScale((boundRect.maxY() - clipRect.Center().y) / (clipRect.maxY() - clipRect.Center().y));

  res.Scale(scale);
  res.SetOrg(clipRect.Center());

  return res;
}

ScreenBase ShrinkAndScaleInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  ScreenBase res = screen;
  m2::RectD globalRect = res.ClipRect();

  //double maxOverscrollRight = boundRect.maxX() + (screen.GetWidth() / 2);
  //double maxOverscrollLeft = boundRect.minX() - (screen.GetWidth() / 2);

  m2::RectD boundRectWOverscroll = m2::RectD(boundRect.minX() - (globalRect.SizeX() / 2),
  boundRect.minY(), boundRect.maxX() + (globalRect.SizeX() / 2), boundRect.maxY());

  m2::PointD newOrg = res.GetOrg();
  double scale = 1;
  double offs = 0;

  if (globalRect.minX() < boundRectWOverscroll.minX())
  {
    offs = boundRectWOverscroll.minX() - globalRect.minX();
    globalRect.Offset(offs, 0);
    newOrg.x += offs;

    if (globalRect.maxX() > boundRectWOverscroll.maxX())
    {
      double k = boundRectWOverscroll.SizeX() / globalRect.SizeX();
      scale /= k;
      /// scaling always occur pinpointed to the rect center...
      globalRect.Scale(k);
      /// ...so we should shift a rect after scale
      globalRect.Offset(boundRectWOverscroll.minX() - globalRect.minX(), 0);
    }
  }

  if (globalRect.maxX() > boundRectWOverscroll.maxX())
  {
    offs = boundRectWOverscroll.maxX() - globalRect.maxX();
    globalRect.Offset(offs, 0);
    newOrg.x += offs;

    if (globalRect.minX() < boundRectWOverscroll.minX())
    {
      double k = boundRectWOverscroll.SizeX() / globalRect.SizeX();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(boundRectWOverscroll.maxX() - globalRect.maxX(), 0);
    }
  }

  if (globalRect.minY() < boundRectWOverscroll.minY())
  {
    offs = boundRectWOverscroll.minY() - globalRect.minY();
    globalRect.Offset(0, offs);
    newOrg.y += offs;

    if (globalRect.maxY() > boundRectWOverscroll.maxY())
    {
      double k = boundRectWOverscroll.SizeY() / globalRect.SizeY();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(0, boundRectWOverscroll.minY() - globalRect.minY());
    }
  }

  if (globalRect.maxY() > boundRectWOverscroll.maxY())
  {
    offs = boundRectWOverscroll.maxY() - globalRect.maxY();
    globalRect.Offset(0, offs);
    newOrg.y += offs;

    if (globalRect.minY() < boundRectWOverscroll.minY())
    {
      double k = boundRectWOverscroll.SizeY() / globalRect.SizeY();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(0, boundRectWOverscroll.maxY() - globalRect.maxY());
    }
  }

  res.SetOrg(globalRect.Center());
  res.Scale(scale);

  return res;
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
      tmp = ShrinkInto(tmp, worldR);
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
    tmp = ScaleInto(tmp, worldR);

  screen = tmp;
  return true;
}

}  // namespace df
