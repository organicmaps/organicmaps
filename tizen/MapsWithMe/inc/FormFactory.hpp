#pragma once
#include <FUi.h>

// Use 'extern' to eliminate duplicate data allocation.
extern const wchar_t * FORM_MAP;
extern const wchar_t * FORM_SETTINGS;
extern const wchar_t * FORM_DOWNLOAD_GROUP;
extern const wchar_t * FORM_DOWNLOAD_COUNTRY;
extern const wchar_t * FORM_DOWNLOAD_REGION;
extern const wchar_t * FORM_ABOUT;
extern const wchar_t * FORM_SEARCH;
extern const wchar_t * FORM_BMCATEGORIES;
extern const wchar_t * FORM_SELECT_BM_CATEGORY;
extern const wchar_t * FORM_SELECT_COLOR;
extern const wchar_t * FORM_CATEGORY;
extern const wchar_t * FORM_SHARE_POSITION;
extern const wchar_t * FORM_LICENSE;

class FormFactory
  : public Tizen::Ui::Scenes::IFormFactory
{
public:
  FormFactory(void);
  virtual ~FormFactory(void);

  virtual Tizen::Ui::Controls::Form * CreateFormN(Tizen::Base::String const & formId, Tizen::Ui::Scenes::SceneId const & sceneId);
};
