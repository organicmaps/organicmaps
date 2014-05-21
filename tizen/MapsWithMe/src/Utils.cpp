#include "Utils.hpp"
#include <FBase.h>
#include <FAppApp.h>
#include <FApp.h>
#include <FUi.h>
#include "../../../platform/tizen_utils.hpp"

using Tizen::App::App;
using Tizen::App::AppResource;
using Tizen::Base::String;
using namespace Tizen::Ui::Controls;

Tizen::Base::String GetString(const wchar_t * IDC)
{
  AppResource * pApp = App::GetInstance()->GetAppResource();
  Tizen::Base::String res;
  pApp->GetString(IDC, res);
  return res;
}

Tizen::Base::String FormatString1(const wchar_t * IDC, Tizen::Base::String const & param1)
{
  String formatter = GetString(IDC);
  char buff[1000];
  sprintf(buff, FromTizenString(formatter).c_str(), FromTizenString(param1).c_str());
  return String(buff);
}

Tizen::Base::String FormatString2(const wchar_t * IDC, Tizen::Base::String const & param1, Tizen::Base::String const & param2)
{
  String formatter = GetString(IDC);
  char buff[1000];
  sprintf(buff, FromTizenString(formatter).c_str(), FromTizenString(param1).c_str(), FromTizenString(param2).c_str());
  return String(buff);
}

bool MessageBoxAsk(Tizen::Base::String const & title, Tizen::Base::String const & msg)
{
  MessageBox messageBox;
  messageBox.Construct(title, msg, MSGBOX_STYLE_YESNO, 5000);
  int modalResult = 0;
  messageBox.ShowAndWait(modalResult);
  return modalResult == MSGBOX_RESULT_YES;
}

void MessageBoxOk(Tizen::Base::String const & title, Tizen::Base::String const & msg)
{
  MessageBox messageBox;
   messageBox.Construct(title, msg, MSGBOX_STYLE_OK, 5000);
   int modalResult = 0;
   messageBox.ShowAndWait(modalResult);
}
