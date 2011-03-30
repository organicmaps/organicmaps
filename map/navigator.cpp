#include "navigator.hpp"
#include "settings.hpp"

#include "../indexer/cell_id.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../coding/streams_sink.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/transformations.hpp"
#include "../geometry/point2d.hpp"

#include "../base/std_serialization.hpp"
#include "../base/logging.hpp"

#include "../base/start_mem_debug.hpp"

Navigator::Navigator()
  : m_orientation(EOrientation0)
{
}

Navigator::Navigator(ScreenBase const & screen)
  : m_StartScreen(screen),
  m_Screen(screen),
  m_InAction(false),
  m_orientation(EOrientation0)
{
}

void Navigator::SetMinScreenParams(unsigned pxMinWidth, double metresMinWidth)
{
  m_pxMinWidth = pxMinWidth;
  m_metresMinWidth = metresMinWidth;
}

void Navigator::SetFromRect(m2::RectD const & r)
{
  m_Screen.SetFromRect(r);
  if (!m_InAction)
    m_StartScreen.SetFromRect(r);
}

void Navigator::CenterViewport(m2::PointD const & p)
{
  /// Rounding center point to a pixel
  /// boundary to obtain crisp centered picture.
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
  Settings::Set("ScreenClipRect", m_Screen.ClipRect());
}

bool Navigator::LoadState()
{
  m2::RectD rect;
  if (!Settings::Get("ScreenClipRect", rect))
    return false;
  SetFromRect(rect);
  return true;
}

void Navigator::OnSize(int x0, int y0, int w, int h)
{
  m_Screen.OnSize(x0, y0, w, h);

  m2::RectD globalRect = m_Screen.GlobalRect();
  m2::RectD tmpRect = globalRect;

  /// trying to shift global rect a bit

  if (globalRect.minX() < MercatorBounds::minX)
  {
    globalRect.Offset(m2::PointD(MercatorBounds::minX - globalRect.minX(), 0));

    if (globalRect.maxX() > MercatorBounds::maxX)
    {
      double k = (globalRect.Center().x - MercatorBounds::maxX) / (globalRect.Center().x - globalRect.maxX());
      globalRect.Scale(k);
    }
  }

  if (globalRect.maxX() > MercatorBounds::maxX)
  {
    globalRect.Offset(m2::PointD(MercatorBounds::maxX - globalRect.maxX(), 0));
    if (globalRect.minX() < MercatorBounds::minX)
    {
      double k = (globalRect.Center().x - MercatorBounds::minX) / (globalRect.Center().x - globalRect.minX());
      globalRect.Scale(k);
    }
  }

  if (globalRect.minY() < MercatorBounds::minY)
  {
    globalRect.Offset(m2::PointD(MercatorBounds::minY - globalRect.minY(), 0));

    if (globalRect.maxY() > MercatorBounds::maxY)
    {
      double k = (globalRect.Center().y - MercatorBounds::maxY) / (globalRect.Center().y - globalRect.maxY());
      globalRect.Scale(k);
    }
  }

  if (globalRect.maxY() > MercatorBounds::maxY)
  {
    globalRect.Offset(m2::PointD(MercatorBounds::maxY - globalRect.maxY(), 0));
    if (globalRect.minY() < MercatorBounds::minY)
    {
      double k = (globalRect.Center().y - MercatorBounds::minY) / (globalRect.Center().y - globalRect.minY());
      globalRect.Scale(k);
    }
  }

  m_Screen.SetFromRect(globalRect);

  if (!m_InAction)
    m_StartScreen.OnSize(x0, y0, w, h);
}

void Navigator::StartDrag(m2::PointD const & pt, double /*timeInSec*/)
{
  //LOG(LDEBUG, (pt.x, pt.y));
  m_StartPt1 = m_LastPt1 = pt;
  m_StartScreen = m_Screen;
  m_InAction = true;
}

void Navigator::DoDrag(m2::PointD const & pt, double /*timeInSec*/)
{
  if (m_LastPt1 == pt)
    return;
  //LOG(LDEBUG, (pt.x, pt.y));
  //m_Screen = m_StartScreen;

  ScreenBase tmp = m_StartScreen;

  int dx = pt.x - m_StartPt1.x;
  int dy = pt.y - m_StartPt1.y;

  tmp.Move(dx, 0);
  if (!CheckBorders(tmp))
    dx = 0;
  tmp = m_StartScreen;
  tmp.Move(0, dy);
  if (!CheckBorders(tmp))
    dy = 0;

  tmp = m_StartScreen;
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
  // TODO: animate.

  // Ensure that final pos is reached.
  DoDrag(pt, timeInSec);
  //ASSERT(m_LastPt1 == pt, (m_LastPt1.x, m_LastPt1.y, pt.x, pt.y));
  //LOG(LDEBUG, (pt.x, pt.y));
  m_InAction = false;
//  m_StartScreen = m_Screen;
}

bool Navigator::InAction() const
{
  return m_InAction;
}

void Navigator::StartScale(m2::PointD const & pt1, m2::PointD const & pt2, double /*timeInSec*/)
{
  //LOG(LDEBUG, (pt1.x, pt1.y, pt2.x, pt2.y));
  m_StartScreen = m_Screen;
  m_StartPt1 = pt1;
  m_StartPt2 = pt2;

  m_InAction = true;
}

void Navigator::ScaleToPoint(m2::PointD const & pt, double factor, double /*timeInSec*/)
{
  m2::PointD startPt;
  m2::PointD endPt;

  // pt is in x0, x0 + width

  if (pt.x != m_Screen.PixelRect().maxX())
  {
    // start scaling point is 1 / factor way between pt and right border
    startPt.x = pt.x + ((m_Screen.PixelRect().maxX() - pt.x) / factor);
    endPt.x = m_Screen.PixelRect().maxX();
  }
  else
  {
    // start scaling point is 1 - 1/factor way between left border and pt
    startPt.x = pt.x + (m_Screen.PixelRect().minX() - pt.x) / factor;
    endPt.x = m_Screen.PixelRect().minX();
  }

  if (pt.y != m_Screen.PixelRect().maxY())
  {
    startPt.y = pt.y + ((m_Screen.PixelRect().maxY() - pt.y) / factor);
    endPt.y = m_Screen.PixelRect().maxY();
  }
  else
  {
    startPt.y = pt.y + (m_Screen.PixelRect().minY() - pt.y) / factor;
    endPt.y = m_Screen.PixelRect().minY();
  }

  ScaleImpl(pt, endPt, pt, startPt, factor > 1);
}

bool Navigator::CheckMaxScale(ScreenBase const & screen)
{
  m2::RectD r = screen.ClipRect();
  // multiple by 2 to allow scale on zero level
  double const maxSize = (MercatorBounds::maxX - MercatorBounds::minX);
  return (r.SizeX() <= maxSize || r.SizeY() <= maxSize);
}

bool Navigator::CheckMinScale(ScreenBase const & screen)
{
  m2::PointD const pt0 = screen.GlobalRect().Center();
  m2::PointD const pt1 = screen.PtoG(screen.GtoP(pt0) + m2::PointD(m_pxMinWidth, 0));
  double lonDiff = fabs(MercatorBounds::XToLon(pt1.x) - MercatorBounds::XToLon(pt0.x));
  double metresDiff = lonDiff / MercatorBounds::degreeInMetres;
  return metresDiff >= m_metresMinWidth - 1;
}

bool Navigator::CheckBorders(ScreenBase const & screen)
{
  m2::RectD ScreenBounds = screen.GlobalRect();

  m2::RectD WorldBounds(MercatorBounds::minX, MercatorBounds::minY,
                        MercatorBounds::maxX, MercatorBounds::maxY);

  return ScreenBounds.IsRectInside(WorldBounds) || WorldBounds.IsRectInside(ScreenBounds);
}

bool Navigator::ScaleImpl(m2::PointD const & newPt1, m2::PointD const & newPt2,
                          m2::PointD const & oldPt1, m2::PointD const & oldPt2,
                          bool skipMaxScaleAndBordersCheck)
{
  math::Matrix<double, 3, 3> newM = m_Screen.GtoPMatrix() * ScreenBase::CalcTransform(oldPt1, oldPt2, newPt1, newPt2);

  ScreenBase tmp = m_Screen;
  tmp.SetGtoPMatrix(newM);
  tmp.Rotate(tmp.GetAngle());

  if ((!skipMaxScaleAndBordersCheck) && (!CheckMaxScale(tmp) || !CheckBorders(tmp)))
    return false;

  if (!CheckMinScale(tmp))
    return false;

  m_Screen = tmp;

  return true;
}

void Navigator::DoScale(m2::PointD const & pt1, m2::PointD const & pt2, double /*timeInSec*/)
{
  //LOG(LDEBUG, (pt1.x, pt1.y, pt2.x, pt2.y));
  if (m_LastPt1 == pt1 && m_LastPt2 == pt2)
    return;
  if (pt1 == pt2)
    return;

  ScreenBase PrevScreen = m_Screen;

  m_Screen = m_StartScreen;

  if (!ScaleImpl(pt1, pt2, m_StartPt1, m_StartPt2, pt1.Length(pt2) / m_StartPt1.Length(m_StartPt2) > 1))
    m_Screen = PrevScreen;

  m_LastPt1 = pt1;
  m_LastPt2 = pt2;
}

void Navigator::StopScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec)
{
  // Ensure that final pos is reached.
  DoScale(pt1, pt2, timeInSec);
  ASSERT(m_LastPt1 == pt1, (m_LastPt1.x, m_LastPt1.y, pt1.x, pt1.y));
  ASSERT(m_LastPt2 == pt2, (m_LastPt2.x, m_LastPt2.y, pt2.x, pt2.y));
  //LOG(LDEBUG, (pt1.x, pt1.y, pt2.x, pt2.y));
  m_InAction = false;
//  m_StartScreen = m_Screen;
}

void Navigator::Scale(double scale)
{
  ScreenBase tmp = m_Screen;
  tmp.Scale(scale);

  // limit max scale to MercatorBounds
  if (CheckMaxScale(tmp) && CheckMinScale(tmp) && CheckBorders(tmp))
    m_Screen = tmp;
}

void Navigator::Rotate(double angle)
{
  m_Screen.Rotate(angle);
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

void Navigator::SetOrientation(EOrientation orientation)
{
  m_orientation = orientation;
}

EOrientation Navigator::Orientation() const
{
  return m_orientation;
}

m2::PointD const Navigator::OrientPoint(m2::PointD const & pt) const
{
  switch (m_orientation)
  {
  case EOrientation90:
    return m2::PointD(m_Screen.GetWidth() - pt.y, pt.x);
  case EOrientation180:
    return m2::PointD(m_Screen.GetWidth() - pt.x, m_Screen.GetHeight() - pt.y);
  case EOrientation270:
    return m2::PointD(pt.y, m_Screen.GetHeight() - pt.x);
  case EOrientation0:
    return pt;
  };
  return m2::PointD(0, 0);
}

