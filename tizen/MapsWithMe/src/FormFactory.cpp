#include "FormFactory.hpp"
#include "MapsWithMeForm.hpp"
#include "SettingsForm.hpp"
#include "AboutForm.hpp"
#include "SearchForm.hpp"
#include "BMCategoriesForm.hpp"
#include "DownloadCountryForm.hpp"
#include "SelectBMCategoryForm.hpp"
#include "SelectColorForm.hpp"
#include "CategoryForm.hpp"
#include "SharePositionForm.hpp"
#include "LicenseForm.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../base/logging.hpp"

using namespace Tizen::Ui::Scenes;
using namespace Tizen::Base;

// Definitions of extern.
const wchar_t * FORM_MAP = L"FormMap";
const wchar_t * FORM_SETTINGS = L"FormSettings";
const wchar_t * FORM_DOWNLOAD_GROUP = L"FormDownloadGroup";
const wchar_t * FORM_DOWNLOAD_COUNTRY = L"FormDownloadCountry";
const wchar_t * FORM_DOWNLOAD_REGION = L"FormDownloadRegion";
const wchar_t * FORM_ABOUT = L"FormAbout";
const wchar_t * FORM_SEARCH = L"FormSearch";
const wchar_t * FORM_BMCATEGORIES = L"FormCategories";
const wchar_t * FORM_SELECT_BM_CATEGORY = L"FormSelectBMCategory";
const wchar_t * FORM_SELECT_COLOR = L"FormSelectColor";
const wchar_t * FORM_CATEGORY = L"FormCategory";
const wchar_t * FORM_SHARE_POSITION = L"FormSharePosition";
const wchar_t * FORM_LICENSE = L"FormLicense";

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
    pSceneManager->AddSceneEventListener(sceneId, *pMWMForm);
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
  else if (formId == FORM_SEARCH)
  {
    SearchForm * pForm = new SearchForm();
    pForm->Initialize();
    pSceneManager->AddSceneEventListener(sceneId, *pForm);
    pNewForm = pForm;
  }
  else if (formId == FORM_BMCATEGORIES)
  {
    BMCategoriesForm * pForm = new BMCategoriesForm();
    pForm->Initialize();
    pNewForm = pForm;
  }
  else if (formId == FORM_SELECT_BM_CATEGORY)
  {
    SelectBMCategoryForm * pForm = new SelectBMCategoryForm();
    pForm->Initialize();
    pNewForm = pForm;
  }
  else if (formId == FORM_SELECT_COLOR)
  {
    SelectColorForm * pForm = new SelectColorForm();
    pForm->Initialize();
    pNewForm = pForm;
  }
  else if (formId == FORM_CATEGORY)
  {
    CategoryForm * pForm = new CategoryForm();
    pForm->Initialize();
    pSceneManager->AddSceneEventListener(sceneId, *pForm);
    pNewForm = pForm;
  }
  else if (formId == FORM_SHARE_POSITION)
  {
    SharePositionForm * pForm = new SharePositionForm();
    pForm->Initialize();
    pSceneManager->AddSceneEventListener(sceneId, *pForm);
    pNewForm = pForm;
  }
  else if (formId == FORM_LICENSE)
  {
    LicenseForm * pForm = new LicenseForm();
    pForm->Initialize();
    pNewForm = pForm;
  }

  return pNewForm;
}
