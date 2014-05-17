#include "MapsWithMeFrame.h"
#include "SceneRegister.hpp"
#include <FUi.h>

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
  r = pSceneManager->GoForward(ForwardSceneTransition(SCENE_MAP));
  TryReturn(!IsFailed(r), r, "%s", GetErrorMessage(r));


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
