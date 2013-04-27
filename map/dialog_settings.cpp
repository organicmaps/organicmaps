#include "dialog_settings.hpp"

#include "../platform/settings.hpp"

#include "../base/assert.hpp"


namespace dlg_settings
{

/// @note Add new values to the end and do not change order.
//@{
char const * g_arrSettingsName[] =
{
  "FacebookDialog",
  "BuyProDialog" ,
  "AppStoreRate"
};

int g_arrMinForegroundTime[] = { 30 * 60, 60 * 60, 50 * 60};
//@}

string const g_timeInApp = "AllForegroundTime";

void AddTimeToSetting(string const & settingName, double elapsed)
{
  double val = 0;
  (void)Settings::Get(settingName, val);
  Settings::Set(settingName, val + elapsed);
}


bool ShouldShow(DialogT dlg)
{
  string const name = g_arrSettingsName[dlg];

  bool flag = true;
  (void)Settings::Get("ShouldShow" + name, flag);
  if (!flag)
    return false;

  double val = 0;
  (void)Settings::Get(name + "ForegroundTime", val);
  return (val >= g_arrMinForegroundTime[dlg]);
}

void SaveResult(DialogT dlg, ResultT res)
{
  string const name = g_arrSettingsName[dlg];

  switch (res)
  {
  case OK: case Never:
    Settings::Set("ShouldShow" + name, false);
    break;
  case Later:
    Settings::Set(name + "ForegroundTime", 0);
    break;
  default:
    ASSERT ( false, () );
    break;
  }
}

void EnterBackground(double elapsed)
{
  for (int i = 0; i < DlgCount; ++i)
  {
    string const name = string(g_arrSettingsName[i]) + "ForegroundTime";
    AddTimeToSetting(name, elapsed);
  }
  AddTimeToSetting(g_timeInApp, elapsed);
}

}
