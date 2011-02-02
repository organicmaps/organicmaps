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
  m_Screen = m_StartScreen;
  m_Screen.Move(pt.x - m_StartPt1.x, pt.y - m_StartPt1.y);
  m_LastPt1 = pt;
}

void Navigator::StopDrag(m2::PointD const & pt, double timeInSec, bool /*animate*/)
{
  // TODO: animate.

  // Ensure that final pos is reached.
  DoDrag(pt, timeInSec);
  ASSERT(m_LastPt1 == pt, (m_LastPt1.x, m_LastPt1.y, pt.x, pt.y));
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

  ScaleImpl(pt, endPt, pt, startPt);
}

namespace
{
  bool CheckMaxScale(ScreenBase const & screen)
  {
    m2::RectD const r = screen.GlobalRect();

    // multiple by 2 to allow scale on zero level
    double const maxSize = 2.0 * (MercatorBounds::maxX - MercatorBounds::minX);
    return (r.SizeX() <= maxSize || r.SizeY() <= maxSize);
  }
}

void Navigator::ScaleImpl(m2::PointD const & newPt1, m2::PointD const & newPt2,
                          m2::PointD const & oldPt1, m2::PointD const & oldPt2)
{
  math::Matrix<double, 3, 3> newM = m_Screen.GtoPMatrix() * ScreenBase::CalcTransform(oldPt1, oldPt2, newPt1, newPt2);

  ScreenBase tmp = m_Screen;
  tmp.SetGtoPMatrix(newM);
  tmp.Rotate(tmp.GetAngle());

  // limit max scale to MercatorBounds
  if (CheckMaxScale(tmp))
    m_Screen = tmp;
}

void Navigator::DoScale(m2::PointD const & pt1, m2::PointD const & pt2, double /*timeInSec*/)
{
  //LOG(LDEBUG, (pt1.x, pt1.y, pt2.x, pt2.y));
  if (m_LastPt1 == pt1 && m_LastPt2 == pt2)
    return;
  if (pt1 == pt2)
    return;
  m_Screen = m_StartScreen;

  ScaleImpl(pt1, pt2, m_StartPt1, m_StartPt2);

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
  if (CheckMaxScale(tmp))
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

