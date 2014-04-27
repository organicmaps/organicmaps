#include "MapsWithMeForm.hpp"
#include "MapsWithMeApp.h"
#include "Framework.hpp"
#include "../../../map/framework.hpp"
#include "../../../gui/controller.hpp"
#include <FUi.h>
#include <FBase.h>
#include <FMedia.h>
#include <FGraphics.h>

using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Graphics;
using namespace Tizen::Media;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Base::Utility;

MapsWithMeForm::MapsWithMeForm(MapsWithMeApp* pApp)
: m_pApp(pApp)
{
  SetMultipointTouchEnabled(true);
}

MapsWithMeForm::~MapsWithMeForm(void)
{
}

bool MapsWithMeForm::Initialize(void)
{
  LOG(LDEBUG, ("MapsWithMeForm::Initialize"));
  Construct(Tizen::Ui::Controls::FORM_STYLE_NORMAL);
  return true;
}

result MapsWithMeForm::OnInitializing(void)
{
  LOG(LDEBUG, ("MapsWithMeForm::OnInitializing"));
  return E_SUCCESS;
}

void MapsWithMeForm::OnActionPerformed(const Tizen::Ui::Control& source, int actionId)
{
}

result MapsWithMeForm::OnDraw(void)
{
  return m_pApp->Draw();
}

namespace detail
{
  std::vector<std::pair<double, double> > GetTouchedPoints()
  {
    std::vector<std::pair<double, double> > res;
    IListT<TouchEventInfo *> * pList = TouchEventManager::GetInstance()->GetTouchInfoListN();
    if (pList)
    {
      int count = pList->GetCount();
      for (int i = 0; i < count; ++i)
      {

        TouchEventInfo * pTouchInfo;
        pList->GetAt(i, pTouchInfo);
        Point pt = pTouchInfo->GetCurrentPosition();
        res.push_back(std::make_pair(pt.x, pt.y));
      }

      pList->RemoveAll();
      delete pList;
    }
    return res;
  }
}

void MapsWithMeForm::OnTouchPressed(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints();

  ::Framework * pFramework = tizen::Framework::GetInstance();
  if (!pFramework->GetGuiController()->OnTapStarted(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1)
      pFramework->StartDrag(DragEvent(pts[0].first, pts[0].second));
    else if (pts.size() > 1)
      pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
  }

  std::swap(m_prev_pts, pts);
}

void MapsWithMeForm::OnTouchMoved(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints();
  ::Framework * pFramework = tizen::Framework::GetInstance();

  if (!pFramework->GetGuiController()->OnTapMoved(m2::PointD(pts[0].first, pts[0].second)))
  {
    if (pts.size() == 1)
      pFramework->DoDrag(DragEvent(pts[0].first, pts[0].second));
    else if (pts.size() > 1)
    {
      if (m_prev_pts.size() > 1)
      {
        pFramework->DoScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
      }
      else if (!m_prev_pts.empty())
      {
        pFramework->StopDrag(DragEvent(m_prev_pts[0].first, m_prev_pts[0].second));
        pFramework->StartScale(ScaleEvent(pts[0].first, pts[0].second, pts[1].first, pts[1].second));
      }
    }
  }
  std::swap(m_prev_pts, pts);
}

void MapsWithMeForm::OnTouchReleased(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
  std::vector<std::pair<double, double> > pts = detail::GetTouchedPoints();
  ::Framework * pFramework = tizen::Framework::GetInstance();

  //using prev_pts because pts contains not all points
  if (!m_prev_pts.empty())
  {
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

void MapsWithMeForm::OnTouchFocusIn(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
}

void MapsWithMeForm::OnTouchFocusOut(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
}
