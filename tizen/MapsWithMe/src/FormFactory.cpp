#include "FormFactory.hpp"
#include "MapsWithMeForm.hpp"
#include "SettingsForm.hpp"
#include "AboutForm.hpp"
#include "../../../base/logging.hpp"
#include "../../../platform/tizen_string_utils.hpp"

using namespace Tizen::Ui::Scenes;
using namespace Tizen::Base;

// Definitions of extern.
const wchar_t* FORM_MAP = L"FormMap";
const wchar_t* FORM_SETTINGS = L"FormSettings";
const wchar_t* FORM_DOWNLOAD = L"FormDownload";
const wchar_t* FORM_ABOUT = L"FormAbout";


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
    pMWMForm = new (std::nothrow) MapsWithMeForm();
    pMWMForm->Initialize();
    pMWMForm->AddTouchEventListener(*pMWMForm);
    pNewForm = pMWMForm;
  }
  else if (formId == FORM_SETTINGS)
  {
    SettingsForm * pForm = new (std::nothrow) SettingsForm(pMWMForm);
    pForm->Initialize();
    pNewForm = pForm;
  }
  else if (formId == FORM_ABOUT)
  {
    AboutForm * pForm = new (std::nothrow) AboutForm();
    pForm->Initialize();
    pNewForm = pForm;
  }

  return pNewForm;
}
