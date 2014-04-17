#pragma once

#include <FUi.h>

class MapsWithMeApp;

class MapsWithMeForm
: public Tizen::Ui::Controls::Form
{
public:
  MapsWithMeForm(MapsWithMeApp* pApp);
  virtual ~MapsWithMeForm(void);

  virtual result OnDraw(void);

private:
  MapsWithMeApp* __pApp;
};
