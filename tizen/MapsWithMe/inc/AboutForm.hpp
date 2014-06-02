#pragma once

#include <FUi.h>

class AboutForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::IActionEventListener
{
public:
  AboutForm();
  virtual ~AboutForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);

private:
  static const int ID_CLOSE = 101;
};
