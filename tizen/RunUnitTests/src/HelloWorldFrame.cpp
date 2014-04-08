#include "HelloWorldFrame.h"
#include "HelloWorldFormFactory.h"
#include "HelloWorldPanelFactory.h"
#include "AppResourceId.h"

using namespace Tizen::Base;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;

HelloWorldFrame::HelloWorldFrame(void)
{
}

HelloWorldFrame::~HelloWorldFrame(void)
{
}

result HelloWorldFrame::OnInitializing(void)
{
  // Prepare Scene management.
  SceneManager* pSceneManager = SceneManager::GetInstance();
  static HelloWorldFormFactory formFactory;
  static HelloWorldPanelFactory panelFactory;
  pSceneManager->RegisterFormFactory(formFactory);
  pSceneManager->RegisterPanelFactory(panelFactory);
  pSceneManager->RegisterScene(L"workflow");

  // Go to the scene.
  result r = pSceneManager->GoForward(SceneTransitionId(IDSCNT_MAIN_SCENE));

  // TODO: Add your frame initialization code here.
  return r;
}

result HelloWorldFrame::OnTerminating(void)
{
  result r = E_SUCCESS;

  // TODO: Add your frame termination code here.
  return r;
}
