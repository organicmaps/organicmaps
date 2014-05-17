#pragma once
#include <FUi.h>

class MapsWithMeForm;

class SettingsForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::Controls::IFormBackEventListener
{
public:
  SettingsForm(MapsWithMeForm * pMainForm);
  virtual ~SettingsForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);

private:
  static const int ID_BUTTON_STORAGE = 101;
  static const int ID_BUTTON_BACK = 102;
  static const int ID_SCALE_CHECKED   = 201;
  static const int ID_SCALE_UNCHECKED = 202;
  static const int ID_METER_CHECKED = 301;
  static const int ID_FOOT_CHECKED = 302;
  static const int ID_ABOUT_CHECKED = 401;

  MapsWithMeForm * m_pMainForm;
};
