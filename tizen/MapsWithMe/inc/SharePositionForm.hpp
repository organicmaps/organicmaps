#pragma once

#include <FUi.h>

class SharePositionForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::IActionEventListener
, public Tizen::Ui::Scenes::ISceneEventListener
{
public:
  SharePositionForm();
  virtual ~SharePositionForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);
  // ISceneEventListener
  virtual void OnSceneActivatedN(Tizen::Ui::Scenes::SceneId const & previousSceneId,
      Tizen::Ui::Scenes::SceneId const & currentSceneId, Tizen::Base::Collection::IList * pArgs);
  virtual void OnSceneDeactivated(Tizen::Ui::Scenes::SceneId const & currentSceneId,
      Tizen::Ui::Scenes::SceneId const & nextSceneId){}

  enum EActions
  {
    ID_CANCEL,
    ID_SEND_MESSAGE,
    ID_SEND_EMAIL,
    ID_COPY_TO_CLIPBOARD
  };

  bool m_sharePosition;
  Tizen::Base::String m_messageSMS;
  Tizen::Base::String m_messageEmail;
};
