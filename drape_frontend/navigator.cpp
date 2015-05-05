#include "drape_frontend/navigator.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "platform/settings.hpp"

#include "geometry/angles.hpp"
#include "geometry/transformations.hpp"
#include "geometry/point2d.hpp"
#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"

#include "std/function.hpp"
#include "std/bind.hpp"

namespace
{

/// @todo Review this logic in future.
/// Fix bug with floating point calculations (before Android release).
void ReduceRectHack(m2::RectD & r)
{
  r.Inflate(-1.0E-9, -1.0E-9);
}

} // namespace

namespace df
{

Navigator::Navigator()
  : m_InAction(false)
{
}

void Navigator::SetFromRects(m2::AnyRectD const & glbRect, m2::RectD const & pxRect)
{
  m2::RectD const & worldR = df::GetWorldRect();

  m_Screen.SetFromRects(glbRect, pxRect);
  m_Screen = ScaleInto(m_Screen, worldR);

  if (!m_InAction)
  {
    m_StartScreen.SetFromRects(glbRect, pxRect);
    m_StartScreen = ScaleInto(m_StartScreen, worldR);
  }
}

void Navigator::SetFromRect(m2::AnyRectD const & r)
{
  m2::RectD const & worldR = df::GetWorldRect();

  m_Screen.SetFromRect(r);
  m_Screen = ScaleInto(m_Screen, worldR);

  if (!m_InAction)
  {
    m_StartScreen.SetFromRect(r);
    m_StartScreen = ScaleInto(m_StartScreen, worldR);
  }
}

void Navigator::CenterViewport(m2::PointD const & p)
{
  // Rounding center point to a pixel boundary to obtain crisp centered picture.
  m2::PointD pt = m_Screen.GtoP(p);
  pt.x = ceil(pt.x);
  pt.y = ceil(pt.y);

  pt = m_Screen.PtoG(pt);

  m_Screen.SetOrg(pt);
  if (!m_InAction)
    m_StartScreen.SetOrg(pt);
}

void Navigator::SaveState()
{
  Settings::Set("ScreenClipRect", m_Screen.GlobalRect());
}

bool Navigator::LoadState()
{
  m2::AnyRectD rect;
  if (!Settings::Get("ScreenClipRect", rect))
    return false;

  // additional check for valid rect
  if (!df::GetWorldRect().IsRectInside(rect.GetGlobalRect()))
    return false;

  SetFromRect(rect);
  return true;
}

void Navigator::OnSize(int w, int h)
{
  m2::RectD const & worldR = df::GetWorldRect();

  m_Screen.OnSize(0, 0, w, h);
  m_Screen = ShrinkAndScaleInto(m_Screen, worldR);

  m_StartScreen.OnSize(0, 0, w, h);
  m_StartScreen = ShrinkAndScaleInto(m_StartScreen, worldR);
}

m2::PointD Navigator::GtoP(m2::PointD const & pt) const
{
  return m_Screen.GtoP(pt);
}

m2::PointD Navigator::PtoG(m2::PointD const & pt) const
{
  return m_Screen.PtoG(pt);
}

bool Navigator::CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect)
{
  m2::RectD clipRect = screen.ClipRect();
  return (boundRect.SizeX() >= clipRect.SizeX())
      && (boundRect.SizeY() >= clipRect.SizeY());
}

ScreenBase const Navigator::ShrinkInto(ScreenBase const & screen, m2::RectD boundRect)
{
  ReduceRectHack(boundRect);

  ScreenBase res = screen;

  m2::RectD clipRect = res.ClipRect();
  if (clipRect.minX() < boundRect.minX())
    clipRect.Offset(boundRect.minX() - clipRect.minX(), 0);
  if (clipRect.maxX() > boundRect.maxX())
    clipRect.Offset(boundRect.maxX() - clipRect.maxX(), 0);
  if (clipRect.minY() < boundRect.minY())
    clipRect.Offset(0, boundRect.minY() - clipRect.minY());
  if (clipRect.maxY() > boundRect.maxY())
    clipRect.Offset(0, boundRect.maxY() - clipRect.maxY());

  res.SetOrg(clipRect.Center());

  // This assert fails near x = 180 (Philipines).
  //ASSERT ( boundRect.IsRectInside(res.ClipRect()), (clipRect, res.ClipRect()) );
  return res;
}

ScreenBase const Navigator::ScaleInto(ScreenBase const & screen, m2::RectD boundRect)
{
  ReduceRectHack(boundRect);

  ScreenBase res = screen;

  double scale = 1;

  m2::RectD clipRect = res.ClipRect();

  ASSERT(boundRect.IsPointInside(clipRect.Center()), ("center point should be inside boundRect"));

  if (clipRect.minX() < boundRect.minX())
  {
    double k = (boundRect.minX() - clipRect.Center().x) / (clipRect.minX() - clipRect.Center().x);
    scale /= k;
    clipRect.Scale(k);
  }
  if (clipRect.maxX() > boundRect.maxX())
  {
    double k = (boundRect.maxX() - clipRect.Center().x) / (clipRect.maxX() - clipRect.Center().x);
    scale /= k;
    clipRect.Scale(k);
  }
  if (clipRect.minY() < boundRect.minY())
  {
    double k = (boundRect.minY() - clipRect.Center().y) / (clipRect.minY() - clipRect.Center().y);
    scale /= k;
    clipRect.Scale(k);
  }
  if (clipRect.maxY() > boundRect.maxY())
  {
    double k = (boundRect.maxY() - clipRect.Center().y) / (clipRect.maxY() - clipRect.Center().y);
    scale /= k;
    clipRect.Scale(k);
  }

  res.Scale(scale);
  res.SetOrg(clipRect.Center());

  return res;
}

ScreenBase const Navigator::ShrinkAndScaleInto(ScreenBase const & screen, m2::RectD boundRect)
{
  ReduceRectHack(boundRect);

  ScreenBase res = screen;

  m2::RectD globalRect = res.ClipRect();

  m2::PointD newOrg = res.GetOrg();
  double scale = 1;
  double offs = 0;

  if (globalRect.minX() < boundRect.minX())
  {
    offs = boundRect.minX() - globalRect.minX();
    globalRect.Offset(offs, 0);
    newOrg.x += offs;

    if (globalRect.maxX() > boundRect.maxX())
    {
      double k = boundRect.SizeX() / globalRect.SizeX();
      scale /= k;
      /// scaling always occur pinpointed to the rect center...
      globalRect.Scale(k);
      /// ...so we should shift a rect after scale
      globalRect.Offset(boundRect.minX() - globalRect.minX(), 0);
    }
  }

  if (globalRect.maxX() > boundRect.maxX())
  {
    offs = boundRect.maxX() - globalRect.maxX();
    globalRect.Offset(offs, 0);
    newOrg.x += offs;

    if (globalRect.minX() < boundRect.minX())
    {
      double k = boundRect.SizeX() / globalRect.SizeX();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(boundRect.maxX() - globalRect.maxX(), 0);
    }
  }

  if (globalRect.minY() < boundRect.minY())
  {
    offs = boundRect.minY() - globalRect.minY();
    globalRect.Offset(0, offs);
    newOrg.y += offs;

    if (globalRect.maxY() > boundRect.maxY())
    {
      double k = boundRect.SizeY() / globalRect.SizeY();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(0, boundRect.minY() - globalRect.minY());
    }
  }

  if (globalRect.maxY() > boundRect.maxY())
  {
    offs = boundRect.maxY() - globalRect.maxY();
    globalRect.Offset(0, offs);
    newOrg.y += offs;

    if (globalRect.minY() < boundRect.minY())
    {
      double k = boundRect.SizeY() / globalRect.SizeY();
      scale /= k;
      globalRect.Scale(k);
      globalRect.Offset(0, boundRect.maxY() - globalRect.maxY());
    }
  }

  res.SetOrg(globalRect.Center());
  res.Scale(scale);

  return res;
}

void Navigator::StartDrag(m2::PointD const & pt)
{
  m_StartPt1 = m_LastPt1 = pt;
  m_StartScreen = m_Screen;
  m_InAction = true;
}

void Navigator::DoDrag(m2::PointD const & pt)
{
  if (m_LastPt1 == pt)
    return;

  ScreenBase const s = ShrinkInto(m_StartScreen, df::GetWorldRect());

  double dx = pt.x - m_StartPt1.x;
  double dy = pt.y - m_StartPt1.y;

  ScreenBase tmp = s;
  tmp.Move(dx, 0);
  if (!CheckBorders(tmp))
    dx = 0;

  tmp = s;
  tmp.Move(0, dy);
  if (!CheckBorders(tmp))
    dy = 0;

  tmp = s;
  tmp.Move(dx, dy);

  if (CheckBorders(tmp))
  {
    m_StartScreen = tmp;
    m_StartPt1 = pt;
    m_LastPt1 = pt;
    m_Screen = tmp;
  }
}

void Navigator::StopDrag(m2::PointD const & pt)
{
  DoDrag(pt);
  m_InAction = false;
}

bool Navigator::InAction() const
{
  return m_InAction;
}

void Navigator::StartScale(m2::PointD const & pt1, m2::PointD const & pt2)
{
  m_StartScreen = m_Screen;
  m_StartPt1 = m_LastPt1 = pt1;
  m_StartPt2 = m_LastPt2 = pt2;

  m_DoCheckRotationThreshold = true;
  m_IsRotatingDuringScale = false;
  m_InAction = true;
}

namespace
{
  void CalcScalePoints(m2::PointD const & pt, double factor, m2::RectD const & pxRect,
                       m2::PointD & startPt, m2::PointD & endPt)
  {
    // pt is in x0, x0 + width

    if (pt.x != pxRect.maxX())
    {
      // start scaling point is 1 / factor way between pt and right border
      startPt.x = pt.x + (pxRect.maxX() - pt.x) / factor;
      endPt.x = pxRect.maxX();
    }
    else
    {
      // start scaling point is 1 - 1/factor way between left border and pt
      startPt.x = pt.x + (pxRect.minX() - pt.x) / factor;
      endPt.x = pxRect.minX();
    }

    if (pt.y != pxRect.maxY())
    {
      startPt.y = pt.y + (pxRect.maxY() - pt.y) / factor;
      endPt.y = pxRect.maxY();
    }
    else
    {
      startPt.y = pt.y + (pxRect.minY() - pt.y) / factor;
      endPt.y = pxRect.minY();
    }
  }
}

void Navigator::Scale(m2::PointD const & pt, double factor)
{
  m2::PointD startPt, endPt;
  CalcScalePoints(pt, factor, m_Screen.PixelRect(), startPt, endPt);
  ScaleImpl(pt, endPt, pt, startPt, factor > 1, false);
}

//namespace
//{
//  class ZoomAnim : public anim::Task
//  {
//  public:
//    typedef function<bool (m2::PointD const &, m2::PointD const &,
//                           m2::PointD const &, m2::PointD const &)> TScaleImplFn;
//    ZoomAnim(m2::PointD const & startPt, m2::PointD const & endPt,
//             m2::PointD const & target, TScaleImplFn const & fn, double deltaTime)
//      : m_fn(fn)
//      , m_startTime(0.0)
//      , m_deltaTime(deltaTime)
//    {
//      m_finger1Start = target + (startPt - target);
//      m_prevPt1 = m_finger1Start;
//      m_deltaFinger1 = (endPt - startPt);

//      m_finger2Start = target - (startPt - target);
//      m_prevPt2 = m_finger2Start;
//      m_deltaFinger2 = -(endPt - startPt);
//    }

//    virtual bool IsVisual() const { return true; }

//    void OnStart(double ts)
//    {
//      m_startTime = ts;
//    }

//    void OnStep(double ts)
//    {
//      double elapsed = ts - m_startTime;
//      if (my::AlmostEqual(elapsed, 0.0))
//        return;

//      double t = elapsed / m_deltaTime;
//      if (t > 1.0 || my::AlmostEqual(t, 1.0))
//      {
//        m_fn(m_finger1Start + m_deltaFinger1, m_finger2Start + m_deltaFinger2, m_prevPt1, m_prevPt2);
//        End();
//        return;
//      }

//      m2::PointD const current1 = m_finger1Start + m_deltaFinger1 * t;
//      m2::PointD const current2 = m_finger2Start + m_deltaFinger2 * t;
//      m_fn(current1, current2, m_prevPt1, m_prevPt2);
//      m_prevPt1 = current1;
//      m_prevPt2 = current2;
//    }

//  private:
//    m2::PointD m_prevPt1;
//    m2::PointD m_prevPt2;

//    m2::PointD m_finger1Start;
//    m2::PointD m_deltaFinger1;
//    m2::PointD m_finger2Start;
//    m2::PointD m_deltaFinger2;

//    TScaleImplFn m_fn;
//    double m_startTime;
//    double m_deltaTime;
//  };
//}

bool Navigator::CheckMinScale(ScreenBase const & screen) const
{
  m2::RectD const & r = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();

  return (r.SizeX() <= worldR.SizeX() || r.SizeY() <= worldR.SizeY());
}

bool Navigator::CheckMaxScale(ScreenBase const & screen) const
{
  return (df::GetDrawTileScale(screen) <= scales::GetUpperStyleScale());
}

bool Navigator::CheckBorders(ScreenBase const & screen) const
{
  m2::RectD const & r = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();

  return (r.IsRectInside(worldR) || worldR.IsRectInside(r));
}

bool Navigator::ScaleImpl(m2::PointD const & newPt1, m2::PointD const & newPt2,
                          m2::PointD const & oldPt1, m2::PointD const & oldPt2,
                          bool skipMinScaleAndBordersCheck,
                          bool doRotateScreen)
{
  math::Matrix<double, 3, 3> newM = m_Screen.GtoPMatrix() * ScreenBase::CalcTransform(oldPt1, oldPt2, newPt1, newPt2);

  double oldAngle = m_Screen.GetAngle();
  ScreenBase tmp = m_Screen;
  tmp.SetGtoPMatrix(newM);
  if (!doRotateScreen)
    tmp.Rotate(-(tmp.GetAngle() - oldAngle));

  if (!skipMinScaleAndBordersCheck && !CheckMinScale(tmp))
    return false;

  m2::RectD const & worldR = df::GetWorldRect();

  if (!skipMinScaleAndBordersCheck && !CheckBorders(tmp))
  {
    if (CanShrinkInto(tmp, worldR))
      tmp = ShrinkInto(tmp, worldR);
    else
      return false;
  }

  if (!CheckMaxScale(tmp))
    return false;

  // re-checking the borders, as we might violate them a bit (don't know why).
  if (!CheckBorders(tmp))
    tmp = ScaleInto(tmp, worldR);

  m_Screen = tmp;
  return true;
}

void Navigator::DoScale(m2::PointD const & pt1, m2::PointD const & pt2)
{
  if (m_LastPt1 == pt1 && m_LastPt2 == pt2)
    return;
  if (pt1 == pt2)
    return;

  ScreenBase PrevScreen = m_Screen;
  m_Screen = m_StartScreen;

  // Checking for rotation threshold.
  if (m_DoCheckRotationThreshold)
  {
    double s = pt1.Length(pt2) / m_StartPt1.Length(m_StartPt2);
    double a = ang::AngleTo(pt1, pt2) - ang::AngleTo(m_StartPt1, m_StartPt2);

    double aThresh = 10.0 / 180.0 * math::pi;
    double sThresh = 1.2;

    bool isScalingInBounds = (s > 1 / sThresh) && (s < sThresh);
    bool isRotationOutBounds = fabs(a) > aThresh;

    if (isScalingInBounds)
    {
      if (isRotationOutBounds)
      {
        m_IsRotatingDuringScale = true;
        m_DoCheckRotationThreshold = false;
      }
    }
    else
    {
      m_IsRotatingDuringScale = false;
      m_DoCheckRotationThreshold = false;
    }
  }

  m_Screen = PrevScreen;

  if (!ScaleImpl(pt1, pt2,
                 m_LastPt1, m_LastPt2,
                 pt1.Length(pt2) > m_LastPt1.Length(m_LastPt2),
                 m_IsRotatingDuringScale))
  {
    m_Screen = PrevScreen;
  }

  m_LastPt1 = pt1;
  m_LastPt2 = pt2;
}

void Navigator::StopScale(m2::PointD const & pt1, m2::PointD const & pt2)
{
  DoScale(pt1, pt2);

  ASSERT_EQUAL(m_LastPt1, pt1, ());
  ASSERT_EQUAL(m_LastPt2, pt2, ());

  m_InAction = false;
}

bool Navigator::IsRotatingDuringScale() const
{
  return m_IsRotatingDuringScale;
}

m2::AnyRectD ToRotated(Navigator const & navigator, m2::RectD const & rect)
{
  double const dx = rect.SizeX();
  double const dy = rect.SizeY();

  return m2::AnyRectD(rect.Center(),
                      navigator.Screen().GetAngle(),
                      m2::RectD(-dx/2, -dy/2, dx/2, dy/2));
}

void CheckMinGlobalRect(m2::RectD & rect)
{
  m2::RectD const minRect = df::GetRectForDrawScale(scales::GetUpperStyleScale(), rect.Center());
  if (minRect.IsRectInside(rect))
    rect = minRect;
}

void CheckMinMaxVisibleScale(TIsCountryLoaded const & fn, m2::RectD & rect, int maxScale)
{
  CheckMinGlobalRect(rect);

  m2::PointD const c = rect.Center();
  int const worldS = scales::GetUpperWorldScale();

  int scale = df::GetDrawTileScale(rect);
  if (scale > worldS && !fn(c))
  {
    // country is not loaded - limit on world scale
    rect = df::GetRectForDrawScale(worldS, c);
    scale = worldS;
  }

  if (maxScale != -1 && scale > maxScale)
  {
    // limit on passed maximal scale
    rect = df::GetRectForDrawScale(maxScale, c);
  }
}

} // namespace df
