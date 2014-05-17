#include "SettingsForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "Framework.hpp"
#include "AppResourceId.h"
#include "../../../base/logging.hpp"
#include "../../../platform/settings.hpp"
#include "../../../map/framework.hpp"

using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Graphics;
using namespace Tizen::Media;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Base::Utility;
using namespace Tizen::App;
using namespace Tizen::Ui::Scenes;

SettingsForm::SettingsForm(MapsWithMeForm * pForm)
: m_pMainForm(pForm)
{
}

SettingsForm::~SettingsForm(void)
{
}

bool SettingsForm::Initialize(void)
{
  LOG(LDEBUG, ("SettingsForm::Initialize"));
  Construct("IDF_SETTINGS_FORM");
  return true;
}

result SettingsForm::OnInitializing(void)
{
  Settings::Units u = Settings::Metric;
  Settings::Get("Units", u);

  CheckButton * pMetersButton = static_cast<CheckButton *>(GetControl(IDC_METERS_CHECKBUTTON, true));
  pMetersButton->SetActionId(ID_METER_CHECKED, -1);
  pMetersButton->AddActionEventListener(*this);

  CheckButton * pFootsButton = static_cast<CheckButton *>(GetControl(IDC_FOOTS_CHECKBUTTON, true));
  pFootsButton->SetActionId(ID_FOOT_CHECKED, -1);
  pFootsButton->AddActionEventListener(*this);

  pMetersButton->SetSelected(u == Settings::Metric);
  pFootsButton->SetSelected(u == Settings::Foot);

  bool bEnableScaleButtons = true;
  Settings::Get("ScaleButtons", bEnableScaleButtons);
  CheckButton * pScaleButton = static_cast<CheckButton *>(GetControl(IDC_ENABLE_SCALE_BUTTONS_CB, true));
  pScaleButton->SetActionId(ID_SCALE_CHECKED, ID_SCALE_UNCHECKED);
  pScaleButton->SetSelected(bEnableScaleButtons);
  pScaleButton->AddActionEventListener(*this);

  Button * pAboutButton = static_cast<Button *>(GetControl(IDC_ABOUT_BUTTON, true));
  pAboutButton->SetActionId(ID_ABOUT_CHECKED);
  pAboutButton->AddActionEventListener(*this);

  SetFormBackEventListener(this);
  return E_SUCCESS;
}

void SettingsForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
    switch(actionId)
    {
      case ID_METER_CHECKED:
      {
        Settings::Set("Units", Settings::Metric);
        ::Framework * pFramework = tizen::Framework::GetInstance();
        pFramework->SetupMeasurementSystem();
        break;
      }
      case ID_FOOT_CHECKED:
      {
        Settings::Set("Units", Settings::Foot);
        ::Framework * pFramework = tizen::Framework::GetInstance();
        pFramework->SetupMeasurementSystem();
        break;
      }
      case ID_SCALE_CHECKED:
      {
        Settings::Set("ScaleButtons", true);
        break;
      }
      case ID_SCALE_UNCHECKED:
      {
        Settings::Set("ScaleButtons", false);
        break;
      }
      case ID_ABOUT_CHECKED:
      {
        SceneManager * pSceneManager = SceneManager::GetInstance();
        pSceneManager->GoForward(ForwardSceneTransition(SCENE_ABOUT, SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_ADD_HISTORY, SCENE_DESTROY_OPTION_DESTROY));
        break;
      }
      case ID_BUTTON_BACK:
      {
        SceneManager * pSceneManager = SceneManager::GetInstance();
        m_pMainForm->UpdateButtons();
        pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
        break;
      }
    }
    Invalidate(true);
}

void SettingsForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  m_pMainForm->UpdateButtons();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}
