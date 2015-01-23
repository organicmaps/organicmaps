#include "map/navigator.hpp"

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
}

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

double Navigator::ComputeMoveSpeed(m2::PointD const & /*p0*/, m2::PointD const & /*p1*/) const
{
  // we think that with fixed time interval will be better
  return 0.2;//max(0.5, min(0.5, 0.5 * GtoP(p0).Length(GtoP(p1)) / 50.0));
}

void Navigator::OnSize(int x0, int y0, int w, int h)
{
  m2::RectD const & worldR = df::GetWorldRect();

  m_Screen.OnSize(x0, y0, w, h);
  m_Screen = ShrinkAndScaleInto(m_Screen, worldR);

  m_StartScreen.OnSize(x0, y0, w, h);
  m_StartScreen = ShrinkAndScaleInto(m_StartScreen, worldR);
}

m2::PointD Navigator::GtoP(m2::PointD const & pt) const
{
  return m_Screen.GtoP(pt) - ShiftPoint(m2::PointD(0.0, 0.0));
}

m2::PointD Navigator::PtoG(m2::PointD const & pt) const
{
  return m_Screen.PtoG(ShiftPoint(pt));
}

void Navigator::GetTouchRect(m2::PointD const & pixPoint, double pixRadius, m2::AnyRectD & glbRect) const
{
  m_Screen.GetTouchRect(ShiftPoint(pixPoint), pixRadius, glbRect);
}

void Navigator::GetTouchRect(m2::PointD const & pixPoint,
                             double pxWidth, double pxHeight,
                             m2::AnyRectD & glbRect) const
{
  m_Screen.GetTouchRect(ShiftPoint(pixPoint), pxWidth, pxHeight, glbRect);
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

void Navigator::StartRotate(double a, double /*timeInSec*/)
{
  m_StartAngle = a;
  m_StartScreen = m_Screen;
  m_InAction = true;
}

void Navigator::DoRotate(double a, double /*timeInSec*/)
{
  ScreenBase tmp = m_StartScreen;
  tmp.Rotate(a - m_StartAngle);
  m_StartAngle = a;
  m_Screen = tmp;
  m_StartScreen = tmp;
}

void Navigator::StopRotate(double a, double timeInSec)
{
  DoRotate(a, timeInSec);
  m_InAction = false;
}

m2::PointD Navigator::ShiftPoint(m2::PointD const & pt) const
{
  m2::RectD const & pxRect = m_Screen.PixelRect();
  return pt + m2::PointD(pxRect.minX(), pxRect.minY());
}

void Navigator::StartDrag(m2::PointD const & pt, double /*timeInSec*/)
{
  m_StartPt1 = m_LastPt1 = pt;
  m_StartScreen = m_Screen;
  m_InAction = true;
}

void Navigator::DoDrag(m2::PointD const & pt, double /*timeInSec*/)
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

void Navigator::StopDrag(m2::PointD const & pt, double timeInSec, bool /*animate*/)
{
  DoDrag(pt, timeInSec);
  m_InAction = false;
}

bool Navigator::InAction() const
{
  return m_InAction;
}

void Navigator::StartScale(m2::PointD const & pt1, m2::PointD const & pt2, double /*timeInSec*/)
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

void Navigator::ScaleToPoint(m2::PointD const & pt, double factor, double /*timeInSec*/)
{
  m2::PointD startPt, endPt;
  CalcScalePoints(pt, factor, m_Screen.PixelRect(), startPt, endPt);
  ScaleImpl(pt, endPt, pt, startPt, factor > 1, false);
}

namespace
{
  class ZoomAnim : public anim::Task
  {
  public:
    typedef function<bool (m2::PointD const &, m2::PointD const &,
                           m2::PointD const &, m2::PointD const &)> TScaleImplFn;
    ZoomAnim(m2::PointD const & startPt, m2::PointD const & endPt,
             m2::PointD const & target, TScaleImplFn const & fn, double deltaTime)
      : m_fn(fn)
      , m_startTime(0.0)
      , m_deltaTime(deltaTime)
    {
      m_finger1Start = target + (startPt - target);
      m_prevPt1 = m_finger1Start;
      m_deltaFinger1 = (endPt - startPt);

      m_finger2Start = target - (startPt - target);
      m_prevPt2 = m_finger2Start;
      m_deltaFinger2 = -(endPt - startPt);
    }

    virtual bool IsVisual() const { return true; }

    void OnStart(double ts)
    {
      m_startTime = ts;
    }

    void OnStep(double ts)
    {
      double elapsed = ts - m_startTime;
      if (my::AlmostEqualULPs(elapsed, 0.0))
        return;
      
      double t = elapsed / m_deltaTime;
      if (t > 1.0 || my::AlmostEqualULPs(t, 1.0))
      {
        m_fn(m_finger1Start + m_deltaFinger1, m_finger2Start + m_deltaFinger2, m_prevPt1, m_prevPt2);
        End();
        return;
      }

      m2::PointD const current1 = m_finger1Start + m_deltaFinger1 * t;
      m2::PointD const current2 = m_finger2Start + m_deltaFinger2 * t;
      m_fn(current1, current2, m_prevPt1, m_prevPt2);
      m_prevPt1 = current1;
      m_prevPt2 = current2;
    }

  private:
    m2::PointD m_prevPt1;
    m2::PointD m_prevPt2;

    m2::PointD m_finger1Start;
    m2::PointD m_deltaFinger1;
    m2::PointD m_finger2Start;
    m2::PointD m_deltaFinger2;

    TScaleImplFn m_fn;
    double m_startTime;
    double m_deltaTime;
  };
}

shared_ptr<anim::Task> Navigator::ScaleToPointAnim(m2::PointD const & pt, double factor, double timeInSec)
{
  m2::PointD startPt, endPt;
  CalcScalePoints(pt, factor, m_Screen.PixelRect(), startPt, endPt);
  ZoomAnim * anim = new ZoomAnim(startPt, endPt, pt,
                                 bind(&Navigator::ScaleImpl, this, _1, _2, _3, _4, factor > 1, false),
                                 timeInSec);

  return shared_ptr<anim::Task>(anim);
}

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

void Navigator::DoScale(m2::PointD const & pt1, m2::PointD const & pt2, double /*timeInSec*/)
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

void Navigator::StopScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec)
{
  DoScale(pt1, pt2, timeInSec);

  ASSERT_EQUAL(m_LastPt1, pt1, ());
  ASSERT_EQUAL(m_LastPt2, pt2, ());

  m_InAction = false;
}

bool Navigator::IsRotatingDuringScale() const
{
  return m_IsRotatingDuringScale;
}

void Navigator::Scale(double scale)
{
  ScaleToPoint(m_Screen.PixelRect().Center(), scale, 0);
}

shared_ptr<anim::Task> Navigator::ScaleAnim(double scale)
{
  return ScaleToPointAnim(m_Screen.PixelRect().Center() + m2::PointD(0.0, 300.0), scale, 0.3);
}

void Navigator::Rotate(double angle)
{
  m_Screen.Rotate(angle);
}

void Navigator::SetAngle(double angle)
{
  m_Screen.SetAngle(angle);
}

void Navigator::SetOrg(m2::PointD const & org)
{
  ScreenBase tmp = m_Screen;
  tmp.SetOrg(org);
  if (CheckBorders(tmp))
    m_Screen = tmp;
}

void Navigator::Move(double azDir, double factor)
{
  m2::RectD const r = m_Screen.ClipRect();
  m_Screen.MoveG(m2::PointD(r.SizeX() * factor * cos(azDir), r.SizeY() * factor * sin(azDir)));
}

bool Navigator::Update(double timeInSec)
{
  m_LastUpdateTimeInSec = timeInSec;
  return false;
}

int Navigator::GetDrawScale() const
{
  return df::GetDrawTileScale(m_Screen);
}
