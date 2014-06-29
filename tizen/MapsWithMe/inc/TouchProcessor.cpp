#include "TouchProcessor.hpp"
#include "MapsWithMeForm.hpp"
#include "Framework.hpp"
#include "Constants.hpp"

#include "../../../map/framework.hpp"
#include "../../../gui/controller.hpp"

#include <FGraphics.h>
#include <FUi.h>

using namespace Tizen::Ui;
using Tizen::Ui::TouchEventManager;
using namespace Tizen::Graphics;
using namespace Tizen::Base::Runtime;
using namespace consts;
using Tizen::Base::Collection::IListT;

namespace detail
{
TouchProcessor::TPointPairs GetTouchedPoints(Rectangle const & rect)
{
  TouchProcessor::TPointPairs res;
  IListT<TouchEventInfo *> * pList = TouchEventManager::GetInstance()->GetTouchInfoListN();
  if (pList)
  {
    int count = pList->GetCount();
    for (int i = 0; i < count; ++i)
    {
      TouchEventInfo * pTouchInfo;
      pList->GetAt(i, pTouchInfo);
      Point pt = pTouchInfo->GetCurrentPosition();
      res.push_back(std::make_pair(pt.x - rect.x, pt.y - rect.y));
    }

    pList->RemoveAll();
    delete pList;
  }
  return res;
}
}

using namespace detail;

TouchProcessor::TouchProcessor(MapsWithMeForm * pForm)
:m_state(st_empty),
 m_pForm(pForm)
{
  m_timer.Construct(*this);
}

void TouchProcessor::StartMove(TPointPairs const & pts)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (pts.size() == 1)
  {
    pFramework->StartDrag(DragEvent(m_startTouchPoint.first, m_startTouchPoint.second));
    pFramework->DoDrag(DragEvent(pts[0].first, pts[0].second));
    m_state = st_moving;
  }
  else if (pts.size() > 1)
  {
    pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    m_state = st_rotating;
  }
}

void TouchProcessor::OnTouchPressed(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  m_wasLongPress = false;
  m_bWasReleased = false;
  m_prev_pts = detail::GetTouchedPoints(m_pForm->GetClientAreaBounds());
  m_startTouchPoint = make_pair(m_prev_pts[0].first, m_prev_pts[0].second);

  if (m_state == st_waitTimer) // double click
  {
    m_state = st_empty;
    ::Framework * pFramework = tizen::Framework::GetInstance();
    pFramework->ScaleToPoint(ScaleToPointEvent(m_prev_pts[0].first, m_prev_pts[0].second, 2.0));
    m_timer.Cancel();
    return;
  }
  else
  {
    m_state = st_waitTimer;
    m_timer.Start(double_click_timout);
  }
}

void TouchProcessor::OnTimerExpired (Timer &timer)
{
  if (m_state != st_waitTimer)
    LOG(LERROR, ("Undefined behavior, on timer"));

  m_state = st_empty;
  if (m_prev_pts.empty())
    return;
  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (pFramework->GetGuiController()->OnTapStarted(m2::PointD(m_startTouchPoint.first, m_startTouchPoint.second)))
  {
    pFramework->GetGuiController()->OnTapEnded(m2::PointD(m_startTouchPoint.first, m_startTouchPoint.second));
  }
  else if (m_bWasReleased)
  {
    pFramework->GetBalloonManager().OnShowMark(pFramework->GetUserMark(m2::PointD(m_startTouchPoint.first, m_startTouchPoint.second), false));
    m_bWasReleased = false;
  }
  else
  {
    StartMove(m_prev_pts);
  }
}

void TouchProcessor::OnTouchLongPressed(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  m_wasLongPress = true;
  TPointPairs pts = detail::GetTouchedPoints(m_pForm->GetClientAreaBounds());
  if (pts.size() > 0)
  {
    ::Framework * pFramework = tizen::Framework::GetInstance();
    pFramework->GetBalloonManager().OnShowMark(pFramework->GetUserMark(m2::PointD(pts[0].first, pts[0].second), true));
  }
}

void TouchProcessor::OnTouchMoved(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (m_state == st_empty)
  {
    LOG(LERROR, ("Undefined behavior, OnTouchMoved"));
    return;
  }

  TPointPairs pts = detail::GetTouchedPoints(m_pForm->GetClientAreaBounds());
  if (pts.empty())
    return;

  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (m_state == st_waitTimer)
  {
    double dist = sqrt(pow(pts[0].first - m_startTouchPoint.first, 2) + pow(pts[0].second - m_startTouchPoint.second, 2));
    if (dist > 20)
    {
      m_timer.Cancel();
      StartMove(pts);
    }
    else
      return;
  }

  if (pts.size() == 1)
  {
    if (m_state == st_rotating)
    {
      pFramework->StopScale(ScaleEvent(m_prev_pts[0].first, m_prev_pts[0].second, m_prev_pts[1].first, m_prev_pts[1].second));
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (m_state == st_moving)
    {
      pFramework->DoDrag(DragEvent(pts[0].first, pts[0].second));
    }
    m_state = st_moving;
  }
  else if (pts.size() > 1)
  {
    if (m_state == st_rotating)
    {
      pFramework->DoScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
    else if (m_state == st_moving)
    {
      pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
    m_state = st_rotating;
  }
  std::swap(m_prev_pts, pts);
}

void TouchProcessor::OnTouchReleased(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (m_state == st_empty)
  {
    LOG(LERROR, ("Undefined behavior"));
    return;
  }

  if (m_state == st_waitTimer)
  {
    m_bWasReleased = true;
    return;
  }

  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (m_state == st_moving)
  {
    pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
  }
  else if (m_state == st_rotating)
  {
    pFramework->StopScale(ScaleEvent(m_prev_pts[0].first, m_prev_pts[0].second, m_prev_pts[1].first, m_prev_pts[1].second));
  }
  m_state = st_empty;
}
