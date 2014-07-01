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

namespace
{
void GetTouchedPoints(Rectangle const & rect, TouchProcessor::TPointPairs & res)
{
  res.clear();
  IListT<TouchEventInfo *> * pList = TouchEventManager::GetInstance()->GetTouchInfoListN();
  if (pList)
  {
    int count = pList->GetCount();
    for (int i = 0; i < count; ++i)
    {
      TouchEventInfo * pTouchInfo;
      pList->GetAt(i, pTouchInfo);
      Point pt = pTouchInfo->GetCurrentPosition();
      res.push_back(m2::PointD(pt.x - rect.x, pt.y - rect.y));
    }

    pList->RemoveAll();
    delete pList;
  }
}
}

TouchProcessor::TouchProcessor(MapsWithMeForm * pForm)
:m_state(ST_EMPTY),
 m_pForm(pForm)
{
  m_timer.Construct(*this);
}

void TouchProcessor::StartMove(TPointPairs const & pts)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (pts.size() == 1)
  {
    pFramework->StartDrag(DragEvent(m_startTouchPoint.x, m_startTouchPoint.y));
    pFramework->DoDrag(DragEvent(pts[0].x, pts[0].y));
    m_state = ST_MOVING;
  }
  else if (pts.size() > 1)
  {
    pFramework->StartScale(ScaleEvent(pts[0].x, pts[0].y, pts[1].x, pts[1].y));
    m_state = ST_ROTATING;
  }
}

void TouchProcessor::OnTouchPressed(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  m_wasLongPress = false;
  m_bWasReleased = false;
  GetTouchedPoints(m_pForm->GetClientAreaBounds(), m_prev_pts);
  m_startTouchPoint = m_prev_pts[0];

  if (m_state == ST_WAIT_TIMER) // double click
  {
    m_state = ST_EMPTY;
    ::Framework * pFramework = tizen::Framework::GetInstance();
    pFramework->ScaleToPoint(ScaleToPointEvent(m_prev_pts[0].x, m_prev_pts[0].y, 2.0));
    m_timer.Cancel();
    return;
  }
  else
  {
    m_state = ST_WAIT_TIMER;
    m_timer.Start(DoubleClickTimeout);
  }
}

void TouchProcessor::OnTimerExpired (Timer &timer)
{
  if (m_state != ST_WAIT_TIMER)
    LOG(LERROR, ("Undefined behavior, on timer"));

  m_state = ST_EMPTY;
  if (m_prev_pts.empty())
    return;
  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (pFramework->GetGuiController()->OnTapStarted(m_startTouchPoint))
  {
    pFramework->GetGuiController()->OnTapEnded(m_startTouchPoint);
  }
  else if (m_bWasReleased)
  {
    pFramework->GetBalloonManager().OnShowMark(pFramework->GetUserMark(m_startTouchPoint, false));
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
  TPointPairs pts;
  GetTouchedPoints(m_pForm->GetClientAreaBounds(), pts);
  if (pts.size() > 0)
  {
    ::Framework * pFramework = tizen::Framework::GetInstance();
    pFramework->GetBalloonManager().OnShowMark(pFramework->GetUserMark(pts[0], true));
  }
}

void TouchProcessor::OnTouchMoved(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (m_state == ST_EMPTY)
  {
    LOG(LERROR, ("Undefined behavior, OnTouchMoved"));
    return;
  }

  TPointPairs pts;
  GetTouchedPoints(m_pForm->GetClientAreaBounds(), pts);
  if (pts.empty())
    return;

  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (m_state == ST_WAIT_TIMER)
  {
    double dist = sqrt(pow(pts[0].x - m_startTouchPoint.x, 2) + pow(pts[0].y - m_startTouchPoint.y, 2));
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
    if (m_state == ST_ROTATING)
    {
      pFramework->StopScale(ScaleEvent(m_prev_pts[0].x, m_prev_pts[0].y, m_prev_pts[1].x, m_prev_pts[1].y));
      pFramework->StartDrag(DragEvent(pts[0].x, pts[0].y));
    }
    else if (m_state == ST_MOVING)
    {
      pFramework->DoDrag(DragEvent(pts[0].x, pts[0].y));
    }
    m_state = ST_MOVING;
  }
  else if (pts.size() > 1)
  {
    if (m_state == ST_ROTATING)
    {
      pFramework->DoScale(ScaleEvent(pts[0].x, pts[0].y, pts[1].x, pts[1].y));
    }
    else if (m_state == ST_MOVING)
    {
      pFramework->StopDrag(DragEvent(m_prev_pts[0].x, m_prev_pts[0].y));
      pFramework->StartScale(ScaleEvent(pts[0].x, pts[0].y, pts[1].x, pts[1].y));
    }
    m_state = ST_ROTATING;
  }
  std::swap(m_prev_pts, pts);
}

void TouchProcessor::OnTouchReleased(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (m_state == ST_EMPTY)
  {
    LOG(LERROR, ("Undefined behavior"));
    return;
  }

  if (m_state == ST_WAIT_TIMER)
  {
    m_bWasReleased = true;
    return;
  }

  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (m_state == ST_MOVING)
  {
    pFramework->StopDrag(DragEvent(m_prev_pts[0].x, m_prev_pts[0].y));
  }
  else if (m_state == ST_ROTATING)
  {
    pFramework->StopScale(ScaleEvent(m_prev_pts[0].x, m_prev_pts[0].y, m_prev_pts[1].x, m_prev_pts[1].y));
  }
  m_state = ST_EMPTY;
}
