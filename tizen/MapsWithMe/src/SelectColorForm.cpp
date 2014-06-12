#include "SelectColorForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Utils.hpp"
#include "BookMarkUtils.hpp"

#include "../../../base/logging.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"

using namespace Tizen::Base;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace bookmark;

SelectColorForm::SelectColorForm()
{
}

SelectColorForm::~SelectColorForm(void)
{
}

bool SelectColorForm::Initialize(void)
{
  Construct(IDF_SELECT_COLOR_FORM);
  return true;
}

result SelectColorForm::OnInitializing(void)
{
  Button * pButtonRed = static_cast<Button*>(GetControl(IDC_BUTTON_RED, true));
  Button * pButtonBlue = static_cast<Button*>(GetControl(IDC_BUTTON_BLUE, true));
  Button * pButtonBrown = static_cast<Button*>(GetControl(IDC_BUTTON_BROWN, true));
  Button * pButtonGreen = static_cast<Button*>(GetControl(IDC_BUTTON_GREEN, true));
  Button * pButtonOrange = static_cast<Button*>(GetControl(IDC_BUTTON_ORANGE, true));
  Button * pButtonPink = static_cast<Button*>(GetControl(IDC_BUTTON_PINK, true));
  Button * pButtonPurple = static_cast<Button*>(GetControl(IDC_BUTTON_PURPLE, true));
  Button * pButtonYellow = static_cast<Button*>(GetControl(IDC_BUTTON_YELLOW, true));

  pButtonRed->AddActionEventListener(*this);
  pButtonBlue->AddActionEventListener(*this);
  pButtonBrown->AddActionEventListener(*this);
  pButtonGreen->AddActionEventListener(*this);
  pButtonOrange->AddActionEventListener(*this);
  pButtonPink->AddActionEventListener(*this);
  pButtonPurple->AddActionEventListener(*this);
  pButtonYellow->AddActionEventListener(*this);

  pButtonRed->SetActionId(ID_RED);
  pButtonBlue->SetActionId(ID_BLUE);
  pButtonBrown->SetActionId(ID_BROWN);
  pButtonGreen->SetActionId(ID_GREEN);
  pButtonOrange->SetActionId(ID_ORANGE);
  pButtonPink->SetActionId(ID_PINK);
  pButtonPurple->SetActionId(ID_PURPLE);
  pButtonYellow->SetActionId(ID_YELLOW);

  EColor color = BookMarkManager::GetInstance().GetCurBookMarkColor();
  pButtonRed->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_RED ? IDB_COLOR_SELECT_RED : IDB_COLOR_RED));
  pButtonRed->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_RED ? IDB_COLOR_SELECT_RED : IDB_COLOR_RED));
  pButtonBlue->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_BLUE ? IDB_COLOR_SELECT_BLUE : IDB_COLOR_BLUE));
  pButtonBlue->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_BLUE ? IDB_COLOR_SELECT_BLUE : IDB_COLOR_BLUE));
  pButtonBrown->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_BROWN ? IDB_COLOR_SELECT_BROWN : IDB_COLOR_BROWN));
  pButtonBrown->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_BROWN ? IDB_COLOR_SELECT_BROWN : IDB_COLOR_BROWN));
  pButtonGreen->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_GREEN ? IDB_COLOR_SELECT_GREEN : IDB_COLOR_GREEN));
  pButtonGreen->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_GREEN ? IDB_COLOR_SELECT_GREEN : IDB_COLOR_GREEN));
  pButtonOrange->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_ORANGE ? IDB_COLOR_SELECT_ORANGE : IDB_COLOR_ORANGE));
  pButtonOrange->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_ORANGE ? IDB_COLOR_SELECT_ORANGE : IDB_COLOR_ORANGE));
  pButtonPink->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_PINK ? IDB_COLOR_SELECT_PINK : IDB_COLOR_PINK));
  pButtonPink->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_PINK ? IDB_COLOR_SELECT_PINK : IDB_COLOR_PINK));
  pButtonPurple->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_PURPLE ? IDB_COLOR_SELECT_PURPLE : IDB_COLOR_PURPLE));
  pButtonPurple->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_PURPLE ? IDB_COLOR_SELECT_PURPLE : IDB_COLOR_PURPLE));
  pButtonYellow->SetNormalBackgroundBitmap(*GetBitmap(color == CLR_YELLOW ? IDB_COLOR_SELECT_YELLOW : IDB_COLOR_YELLOW));
  pButtonYellow->SetPressedBackgroundBitmap(*GetBitmap(color == CLR_YELLOW ? IDB_COLOR_SELECT_YELLOW : IDB_COLOR_YELLOW));


  SetFormBackEventListener(this);
  return E_SUCCESS;
}

void SelectColorForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  BookMarkManager & bm = BookMarkManager::GetInstance();
  switch(actionId)
  {
    case ID_RED: bm.SetCurBookMarkColor(CLR_RED); break;
    case ID_BLUE: bm.SetCurBookMarkColor(CLR_BLUE); break;
    case ID_BROWN: bm.SetCurBookMarkColor(CLR_BROWN); break;
    case ID_GREEN: bm.SetCurBookMarkColor(CLR_GREEN); break;
    case ID_ORANGE: bm.SetCurBookMarkColor(CLR_ORANGE); break;
    case ID_PINK: bm.SetCurBookMarkColor(CLR_PINK); break;
    case ID_PURPLE: bm.SetCurBookMarkColor(CLR_PURPLE); break;
    case ID_YELLOW: bm.SetCurBookMarkColor(CLR_YELLOW); break;
  }
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

void SelectColorForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}
