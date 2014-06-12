#include "Utils.hpp"
#include "Framework.hpp"

#include <FBase.h>
#include <FAppApp.h>
#include <FApp.h>
#include <FUi.h>
#include <FGraphics.h>

#include "../../../platform/tizen_utils.hpp"
#include "../../../std/map.hpp"
#include "../../../map/user_mark.hpp"
#include "../../../map/framework.hpp"

using namespace Tizen::App;
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

struct BitmapContainer
{
  typedef map<const wchar_t *, Tizen::Graphics::Bitmap const *> TContainer;
  ~BitmapContainer()
  {
    for (TContainer::iterator it = m_Bitmaps.begin(); it != m_Bitmaps.end(); ++it)
      delete it->second;
  }
  TContainer m_Bitmaps;
};

Tizen::Graphics::Bitmap const * GetBitmap(const wchar_t * sBitmapPath)
{
  static BitmapContainer cont;

  BitmapContainer::TContainer::iterator it = cont.m_Bitmaps.find(sBitmapPath);
  if (it == cont.m_Bitmaps.end())
    it = cont.m_Bitmaps.insert(std::make_pair(sBitmapPath, Application::GetInstance()->GetAppResource()->GetBitmapN(sBitmapPath))).first;

  return it->second;
}

::Framework * GetFramework()
{
  return ::tizen::Framework::GetInstance();
}

Tizen::Base::String GetFeature(Tizen::Base::String const & feature)
{
  int ind;
  if (feature.IndexOf(" ", 0, ind) == E_SUCCESS)
  {
    String s;
    feature.SubString(0,ind,s);
    return s;
  }
  return feature;
}
