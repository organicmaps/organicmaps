#include "MapsWithMeFrame.h"

using namespace Tizen::App;
using namespace Tizen::Ui;

MapsWithMeFrame::MapsWithMeFrame(void)
{
}

MapsWithMeFrame::~MapsWithMeFrame(void)
{
}

result MapsWithMeFrame::OnInitializing(void)
{
  result r = E_SUCCESS;

  // TODO: Add your frame initialization code here.

  SetPropagatedKeyEventListener(this);
  return r;
}

result MapsWithMeFrame::OnTerminating(void)
{
  result r = E_SUCCESS;

  // TODO: Add your frame termination code here.
  return r;
}

bool MapsWithMeFrame::OnKeyReleased(Tizen::Ui::Control& source, const Tizen::Ui::KeyEventInfo& keyEventInfo)
{
  KeyCode keyCode = keyEventInfo.GetKeyCode();

  if (keyCode == KEY_BACK)
  {
    UiApp* pApp = UiApp::GetInstance();
    AppAssert(pApp);
    pApp->Terminate();
  }

  return false;
}
