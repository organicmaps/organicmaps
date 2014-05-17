#pragma once
#include <FUi.h>

// Use 'extern' to eliminate duplicate data allocation.
extern const wchar_t* FORM_MAP;
extern const wchar_t* FORM_SETTINGS;
extern const wchar_t* FORM_DOWNLOAD;
extern const wchar_t* FORM_ABOUT;


class FormFactory
  : public Tizen::Ui::Scenes::IFormFactory
{
public:
  FormFactory(void);
  virtual ~FormFactory(void);

  virtual Tizen::Ui::Controls::Form * CreateFormN(Tizen::Base::String const & formId, Tizen::Ui::Scenes::SceneId const & sceneId);
};
