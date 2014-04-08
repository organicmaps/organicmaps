#include "HelloWorldPanelFactory.h"

using namespace Tizen::Ui::Scenes;

HelloWorldPanelFactory::HelloWorldPanelFactory(void)
{
}

HelloWorldPanelFactory::~HelloWorldPanelFactory(void)
{
}

Tizen::Ui::Controls::Panel*
HelloWorldPanelFactory::CreatePanelN(const Tizen::Base::String& panelId, const Tizen::Ui::Scenes::SceneId& sceneId)
{
  SceneManager* pSceneManager = SceneManager::GetInstance();
  AppAssert(pSceneManager);
  Tizen::Ui::Controls::Panel* pNewPanel = null;

  // TODO: Add your panel creation code here
  return pNewPanel;
}
