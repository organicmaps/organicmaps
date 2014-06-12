#include "UserMarkPanel.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Constants.hpp"
#include "BookMarkUtils.hpp"
#include "Utils.hpp"

#include "../../../map/framework.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../base/logging.hpp"

using namespace Tizen::Base;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::Graphics;
using namespace consts;
using namespace bookmark;

UserMarkPanel::UserMarkPanel()
:m_pMainForm(0)
{

}

UserMarkPanel::~UserMarkPanel(void)
{
}

bool UserMarkPanel::Construct(const Tizen::Graphics::FloatRectangle& rect)
{
  Panel::Construct(rect);
  SetBackgroundColor(green);
  m_pButton = new Button;
  m_pButton->Construct(FloatRectangle(rect.width - btnSz - btwWdth, btwWdth, btnSz, btnSz));

  m_pButton->SetActionId(ID_STAR);
  m_pButton->AddActionEventListener(*this);
  AddControl(m_pButton);

  m_pLabel = new Label();
  m_pLabel->Construct(Rectangle(btwWdth, btwWdth, rect.width - 2 * btnSz - btwWdth, markPanelHeight - 2 * btwWdth), "");
  m_pLabel->SetTextColor(white);
  m_pLabel->AddTouchEventListener(*this);
  m_pLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
  m_pLabel->SetTextVerticalAlignment(ALIGNMENT_TOP);
  AddControl(m_pLabel);

  UpdateState();
  return true;
}

void UserMarkPanel::SetMainForm(MapsWithMeForm * pMainForm)
{
  m_pMainForm = pMainForm;
}

void  UserMarkPanel::OnTouchPressed (Tizen::Ui::Control const & source,
    Tizen::Graphics::Point const & currentPosition,
    Tizen::Ui::TouchEventInfo const & touchInfo)
{
  if (m_pMainForm)
    m_pMainForm->ShowBookMarkSplitPanel();
}

void UserMarkPanel::Enable()
{
  String res = GetMarkName(GetCurMark());
   res.Append("\n");
   res.Append(GetMarkType(GetCurMark()));
  m_pLabel->SetText(res);
  SetShowState(true);
  UpdateState();
  Invalidate(true);
}

void UserMarkPanel::Disable()
{
  SetShowState(false);
}

void UserMarkPanel::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  switch(actionId)
  {
    case ID_STAR:
    {
      if (IsBookMark(GetCurMark()))
      {
        BookMarkManager::GetInstance().RemoveCurBookMark();
      }
      else
      {
        BookMarkManager::GetInstance().AddCurMarkToBookMarks();
      }
      UpdateState();
      break;
    }
  }
}

UserMark const * UserMarkPanel::GetCurMark()
{
  return BookMarkManager::GetInstance().GetCurMark();
}

void UserMarkPanel::UpdateState()
{
  if (IsBookMark(GetCurMark()))
  {
    m_pButton->SetNormalBackgroundBitmap(*GetBitmap(IDB_PLACE_PAGE_BUTTON_SELECTED));
    m_pButton->SetPressedBackgroundBitmap(*GetBitmap(IDB_PLACE_PAGE_BUTTON_SELECTED));
  }
  else
  {
    m_pButton->SetNormalBackgroundBitmap(*GetBitmap(IDB_PLACE_PAGE_BUTTON));
    m_pButton->SetPressedBackgroundBitmap(*GetBitmap(IDB_PLACE_PAGE_BUTTON));
  }
  Invalidate(true);
}
