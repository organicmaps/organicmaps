#include "MapsWithMeForm.hpp"
#include "MapsWithMeApp.h"
#include "Framework.hpp"
#include "../../../map/framework.hpp"

MapsWithMeForm::MapsWithMeForm(MapsWithMeApp* pApp)
: m_pApp(pApp)
{
  SetMultipointTouchEnabled(true);
}

MapsWithMeForm::~MapsWithMeForm(void)
{
}

result MapsWithMeForm::OnDraw(void)
{
  return m_pApp->Draw();
}

void MapsWithMeForm::OnTouchPressed(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
  LOG(LINFO, ("OnTouchPressed"));
  ::Framework * pFramework = tizen::Framework::GetInstance();
  pFramework->StartDrag(DragEvent(currentPosition.x, currentPosition.y));
}

void MapsWithMeForm::OnTouchMoved(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
  pFramework->DoDrag(DragEvent(currentPosition.x, currentPosition.y));
}

void MapsWithMeForm::OnTouchReleased(const Tizen::Ui::Control& source,
    const Tizen::Graphics::Point& currentPosition,
    const Tizen::Ui::TouchEventInfo& touchInfo)
{
  ::Framework * pFramework = tizen::Framework::GetInstance();
    pFramework->StopDrag(DragEvent(currentPosition.x, currentPosition.y));
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



