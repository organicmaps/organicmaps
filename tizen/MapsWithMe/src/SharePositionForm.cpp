#include "SharePositionForm.hpp"
#include "SceneRegister.hpp"
#include "MapsWithMeForm.hpp"
#include "AppResourceId.h"
#include "../../../platform/settings.hpp"
#include "../../../platform/tizen_utils.hpp"
#include "../../../base/logging.hpp"
#include <FApp.h>
#include "Utils.hpp"
#include <FBase.h>

using namespace Tizen::Base;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;
using namespace Tizen::App;
using Tizen::Base::Collection::HashMap;

SharePositionForm::SharePositionForm()
{
}

SharePositionForm::~SharePositionForm(void)
{
}

bool SharePositionForm::Initialize(void)
{
  Construct(IDF_SHARE_POSITION_FORM);
  return true;
}

result SharePositionForm::OnInitializing(void)
{
  Button * pMessageButton = static_cast<Button *>(GetControl(IDC_MESSAGE, true));
  pMessageButton->SetActionId(ID_SEND_MESSAGE);
  pMessageButton->AddActionEventListener(*this);

  Button * pCopyButton = static_cast<Button *>(GetControl(IDC_COPY_MARK, true));
  pCopyButton->SetActionId(ID_COPY_TO_CLIPBOARD);
  pCopyButton->AddActionEventListener(*this);

  Button * pCancelButton = static_cast<Button *>(GetControl(IDC_CANCEL, true));
  pCancelButton->SetActionId(ID_CANCEL);
  pCancelButton->AddActionEventListener(*this);

  SetFormBackEventListener(this);
  return E_SUCCESS;
}

void SharePositionForm::OnActionPerformed(Tizen::Ui::Control const & source, int actionId)
{
  switch(actionId)
  {
    case ID_CANCEL:
    {
      SceneManager* pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
      break;
    }
    case ID_COPY_TO_CLIPBOARD:
    {
      Clipboard* pClipboard = Clipboard::GetInstance();
      ClipboardItem item;
      item.Construct(Tizen::Ui::CLIPBOARD_DATA_TYPE_TEXT, m_message);
      pClipboard->CopyItem(item);

      SceneManager* pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
      break;
    }

    case ID_SEND_MESSAGE:
    {
      HashMap extraData;
      extraData.Construct();
      String typeKey = L"http://tizen.org/appcontrol/data/messagetype";
      String typeVal = L"sms";
      String textKey = L"http://tizen.org/appcontrol/data/text";
      String textVal = m_message;
      String toKey = L"http://tizen.org/appcontrol/data/to";
      String toVal = L"";
      extraData.Add(&typeKey, &typeVal);
      extraData.Add(&textKey, &textVal);
      extraData.Add(&toKey, &toVal);
      Tizen::App::AppControl* pAc = AppManager::FindAppControlN(L"tizen.messages",
          L"http://tizen.org/appcontrol/operation/compose");
      if (pAc)
      {
        pAc->Start(null, null, &extraData, null);
        delete pAc;
      }

      SceneManager* pSceneManager = SceneManager::GetInstance();
      pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
      break;
    }
  }
  Invalidate(true);
}

void SharePositionForm::OnFormBackRequested(Tizen::Ui::Controls::Form & source)
{
  SceneManager * pSceneManager = SceneManager::GetInstance();
  pSceneManager->GoBackward(BackwardSceneTransition(SCENE_TRANSITION_ANIMATION_TYPE_RIGHT));
}

void SharePositionForm::OnSceneActivatedN(Tizen::Ui::Scenes::SceneId const & previousSceneId,
    Tizen::Ui::Scenes::SceneId const & currentSceneId, Tizen::Base::Collection::IList* pArgs)
{
  m_message = "";
  if (pArgs != null)
  {
    if (pArgs->GetCount() == 1)
    {
      String * pMessage = dynamic_cast<String *>(pArgs->GetAt(0));
      m_message = *pMessage;
    }
  }

}
