#include "MapsWithMeForm.hpp"
#include "MapsWithMeApp.h"

MapsWithMeForm::MapsWithMeForm(MapsWithMeApp* pApp)
: __pApp(pApp)
{
}

MapsWithMeForm::~MapsWithMeForm(void)
{
}

result MapsWithMeForm::OnDraw(void)
{
  return __pApp->Draw();
}
