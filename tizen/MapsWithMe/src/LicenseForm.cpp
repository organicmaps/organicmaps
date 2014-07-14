#include "LicenseForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "Constants.hpp"
#include "Utils.hpp"

#include "../../../base/logging.hpp"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"

#include <FAppApp.h>

using namespace Tizen::Base;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using Tizen::App::App;

LicenseForm::LicenseForm()
{
}

LicenseForm::~LicenseForm(void)
{
}

bool LicenseForm::Initialize(void)
{
  Construct(IDF_LICENSE_FORM);
  return true;
}

result LicenseForm::OnInitializing(void)
{
  Footer* pFooter = GetFooter();
  pFooter->SetStyle(FOOTER_STYLE_BUTTON_TEXT);

  FooterItem footerItem;
  footerItem.Construct(ID_AGREE);
  footerItem.SetText(GetString(IDS_AGREE));

  FooterItem footerItem2;
  footerItem2.Construct(ID_DISAGREE);
  footerItem2.SetText(GetString(IDS_DISAGREE));

  pFooter->AddItem(footerItem);
  pFooter->AddItem(footerItem2);

  pFooter->AddActionEventListener(*this);
  return E_SUCCESS;
}

void LicenseForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  switch(actionId)
  {
    case ID_AGREE:
    {
      Settings::Set(consts::SETTINGS_MAP_LICENSE, true);
      SceneManager * pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoForward(ForwardSceneTransition(SCENE_MAP,
          SCENE_TRANSITION_ANIMATION_TYPE_LEFT, SCENE_HISTORY_OPTION_NO_HISTORY, SCENE_DESTROY_OPTION_DESTROY));
      break;
    }
    case ID_DISAGREE:
    {
      App::GetInstance()->Terminate();
      break;
    }
  }
}


void LicenseForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  App::GetInstance()->Terminate();
}
