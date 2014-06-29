#include "TouchProcessor.hpp"
#include "MapsWithMeForm.hpp"
#include "Framework.hpp"

#include "../../../map/framework.hpp"
#include "../../../gui/controller.hpp"

#include <FGraphics.h>
#include <FUi.h>

using namespace Tizen::Ui;
using Tizen::Ui::TouchEventManager;
using namespace Tizen::Graphics;
using Tizen::Base::Collection::IListT;

namespace detail
{
typedef vector<pair<double, double> > TPointPairs;

TPointPairs GetTouchedPoints(Rectangle const & rect)
{
  TPointPairs res;
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
:m_pForm(pForm)
{

}
void TouchProcessor::OnTouchPressed(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{

  m_wasLongPress = false;
  TPointPairs pts = detail::GetTouchedPoints(m_pForm->GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();
  //    pFramework->GetBalloonManager().OnShowMark(pFramework->GetUserMark(m2::PointD(pts[0].first, pts[0].second), false));

  m_startTouchPoint = make_pair(pts[0].first, pts[0].second);
  if (!pFramework->GetGuiController()->OnTapStarted(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1)
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    else if (pts.size() > 1)
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
  }

  std::swap(m_prev_pts, pts);
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

void TouchProcessor::OnTouchDoublePressed(Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  pFramework->ScaleDefault(true);

}

void TouchProcessor::OnTouchMoved(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  TPointPairs pts = detail::GetTouchedPoints(m_pForm->GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();

  if (!pFramework->GetGuiController()->OnTapMoved(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1 && m_prev_pts.size() > 1)
    {
      pFramework->StopScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (pts.size() == 1)
    {
      pFramework->DoDrag(DragEvent(pts[0].first, pts[0].second));
    }
    else if (pts.size() > 1 && m_prev_pts.size() == 1)
    {
      pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
    else if (pts.size() > 1)
    {
      pFramework->DoScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
    }
  }
  std::swap(m_prev_pts, pts);
}

void TouchProcessor::OnTouchReleased(Tizen::Ui::Control const & source,
    Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  TPointPairs pts = detail::GetTouchedPoints(m_pForm->GetClientAreaBounds());
  ::Framework * pFramework = tizen::Framework::GetInstance();


  //using prev_pts because pts contains not all points
  if (!m_prev_pts.empty())
  {
    pair<double, double> cur = make_pair(m_prev_pts[0].first, m_prev_pts[0].second);
    double dist = sqrt(pow(cur.first - m_startTouchPoint.first, 2) + pow(cur.second - m_startTouchPoint.second, 2));
    if (dist < 20 && !m_wasLongPress)
    {
      ::Framework * pFramework = tizen::Framework::GetInstance();
      pFramework->GetBalloonManager().OnShowMark(pFramework->GetUserMark(m2::PointD(m_startTouchPoint.first, m_startTouchPoint.second), false));
    }
    if (!pFramework->GetGuiController()->OnTapEnded(m2::PointD(m_prev_pts[0].first, m_prev_pts[0].second)))
    {
      if (m_prev_pts.size() == 1)
        pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
      else if (m_prev_pts.size() > 1)
        pFramework->StopScale(ScaleEvent(m_prev_pts[0].first, m_prev_pts[0].second, m_prev_pts[1].first, m_prev_pts[1].second));
    }
    m_prev_pts.clear();
  }
}
