#include "SceneRegister.hpp"
#include "FormFactory.hpp"
#include <FUi.h>

using namespace Tizen::Ui::Scenes;

// Definitions of extern.
const wchar_t* SCENE_MAP = L"ScnMap";
const wchar_t* SCENE_SETTINGS = L"ScnSettings";
const wchar_t* SCENE_DOWNLOAD = L"ScnDownload";
const wchar_t* SCENE_ABOUT = L"ScnAbout";

SceneRegister::SceneRegister(void)
{
}

SceneRegister::~SceneRegister(void)
{
}

void
SceneRegister::RegisterAllScenes(void)
{
  static const wchar_t* PANEL_BLANK = L"";
  static FormFactory formFactory;

  SceneManager* pSceneManager = SceneManager::GetInstance();
  AppAssert(pSceneManager);
  pSceneManager->RegisterFormFactory(formFactory);

  pSceneManager->RegisterScene(SCENE_MAP, FORM_MAP, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_SETTINGS, FORM_SETTINGS, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_DOWNLOAD, FORM_DOWNLOAD, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_ABOUT, FORM_ABOUT, PANEL_BLANK);
}
