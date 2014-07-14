#include "MapsWithMeFrame.h"
#include "SceneRegister.hpp"
#include "Constants.hpp"

#include <FUi.h>

#include "../../../base/logging.hpp"
#include "../../../platform/settings.hpp"

using namespace Tizen::App;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Scenes;


MapsWithMeFrame::MapsWithMeFrame(void)
{
}

MapsWithMeFrame::~MapsWithMeFrame(void)
{
}

result MapsWithMeFrame::OnInitializing(void)
{
  result r = E_SUCCESS;

  SceneRegister::RegisterAllScenes();
  // Go to the first scene.
  SceneManager* pSceneManager = SceneManager::GetInstance();
  AppAssert(pSceneManager);

  SetPropagatedKeyEventListener(this);

  bool bMapsLicenceAgreement = false;
  if (!Settings::Get(consts::SETTINGS_MAP_LICENSE, bMapsLicenceAgreement) || !bMapsLicenceAgreement)
  {
    r = pSceneManager->GoForward(ForwardSceneTransition(SCENE_LICENSE));
  }
  else
  {
    r = pSceneManager->GoForward(ForwardSceneTransition(SCENE_MAP));
  }

  TryReturn(!IsFailed(r), r, "%s", GetErrorMessage(r));

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
  //  KeyCode keyCode = keyEventInfo.GetKeyCode();
  //
  //  if (keyCode == KEY_BACK)
  //  {
  //    UiApp* pApp = UiApp::GetInstance();
  //    AppAssert(pApp);
  //    pApp->Terminate();
  //  }

  return false;
}
