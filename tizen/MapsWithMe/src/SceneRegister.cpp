#include "SceneRegister.hpp"
#include "FormFactory.hpp"
#include <FUi.h>

using namespace Tizen::Ui::Scenes;

// Definitions of extern.
const wchar_t * SCENE_MAP = L"ScnMap";
const wchar_t * SCENE_SETTINGS = L"ScnSettings";
const wchar_t * SCENE_DOWNLOAD_GROUP = L"ScnDownloadGroup";
const wchar_t * SCENE_DOWNLOAD_COUNTRY = L"ScnDownloadCountry";
const wchar_t * SCENE_DOWNLOAD_REGION = L"ScnDownloadRegion";
const wchar_t * SCENE_ABOUT = L"ScnAbout";
const wchar_t * SCENE_SEARCH = L"ScnSearch";
const wchar_t * SCENE_BMCATEGORIES = L"ScnBMCategories";
const wchar_t * SCENE_SELECT_BM_CATEGORY = L"ScnSelectBMCategory";
const wchar_t * SCENE_SELECT_COLOR = L"ScnSelectColor";
const wchar_t * SCENE_CATEGORY = L"ScnCategory";
const wchar_t * SCENE_SHARE_POSITION = L"ScnSharePosition";
const wchar_t * SCENE_LICENSE = L"ScnLicense";

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
  pSceneManager->RegisterScene(SCENE_DOWNLOAD_GROUP, FORM_DOWNLOAD_GROUP, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_DOWNLOAD_COUNTRY, FORM_DOWNLOAD_COUNTRY, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_DOWNLOAD_REGION, FORM_DOWNLOAD_REGION, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_ABOUT, FORM_ABOUT, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_SEARCH, FORM_SEARCH, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_BMCATEGORIES, FORM_BMCATEGORIES, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_SELECT_BM_CATEGORY, FORM_SELECT_BM_CATEGORY, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_SELECT_COLOR, FORM_SELECT_COLOR, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_CATEGORY, FORM_CATEGORY, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_SHARE_POSITION, FORM_SHARE_POSITION, PANEL_BLANK);
  pSceneManager->RegisterScene(SCENE_LICENSE, FORM_LICENSE, PANEL_BLANK);
}
