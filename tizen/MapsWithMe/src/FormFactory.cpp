#include "FormFactory.hpp"
#include "MapsWithMeForm.hpp"
#include "SettingsForm.hpp"
#include "AboutForm.hpp"
#include "DownloadCountryForm.hpp"
#include "../../../base/logging.hpp"
#include "../../../platform/tizen_utils.hpp"

using namespace Tizen::Ui::Scenes;
using namespace Tizen::Base;

// Definitions of extern.
const wchar_t * FORM_MAP = L"FormMap";
const wchar_t * FORM_SETTINGS = L"FormSettings";
const wchar_t * FORM_DOWNLOAD_GROUP = L"FormDownloadGroup";
const wchar_t * FORM_DOWNLOAD_COUNTRY = L"FormDownloadCountry";
const wchar_t * FORM_DOWNLOAD_REGION = L"FormDownloadRegion";
const wchar_t * FORM_ABOUT = L"FormAbout";


FormFactory::FormFactory(void)
{
}

FormFactory::~FormFactory(void)
{
}

Tizen::Ui::Controls::Form * FormFactory::CreateFormN(String const & formId, SceneId const & sceneId)
{
  Tizen::Ui::Controls::Form * pNewForm = null;
  SceneManager* pSceneManager = SceneManager::GetInstance();
  AppAssert(pSceneManager);

  static MapsWithMeForm * pMWMForm = 0;
  if (formId == FORM_MAP)
  {
    pMWMForm = new MapsWithMeForm();
    pMWMForm->Initialize();
    pMWMForm->AddTouchEventListener(*pMWMForm);
    pNewForm = pMWMForm;
  }
  else if (formId == FORM_SETTINGS)
  {
    SettingsForm * pForm = new SettingsForm(pMWMForm);
    pForm->Initialize();
    pNewForm = pForm;
  }
  else if (formId == FORM_ABOUT)
  {
    AboutForm * pForm = new AboutForm();
    pForm->Initialize();
    pNewForm = pForm;
  }
  else if (formId == FORM_DOWNLOAD_GROUP ||
      formId == FORM_DOWNLOAD_COUNTRY ||
      formId == FORM_DOWNLOAD_REGION)
  {
    DownloadCountryForm * pForm = new DownloadCountryForm();
    pForm->Initialize();
    pSceneManager->AddSceneEventListener(sceneId, *pForm);
    pNewForm = pForm;
  }

  return pNewForm;
}
