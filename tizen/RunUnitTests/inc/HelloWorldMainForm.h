#pragma once
#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

class HelloWorldMainForm: public Tizen::Ui::Controls::Form,
    public Tizen::Ui::IActionEventListener,
    public Tizen::Ui::Controls::IFormBackEventListener,
    public Tizen::Ui::Scenes::ISceneEventListener
{
public:
  HelloWorldMainForm(void);
  virtual ~HelloWorldMainForm(void);
  bool Initialize(void);

private:
  virtual result OnInitializing(void);
  virtual result OnTerminating(void);
  virtual void OnActionPerformed(const Tizen::Ui::Control& source, int actionId);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form& source);
  virtual void OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
      const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs);
  virtual void OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId& currentSceneId,
      const Tizen::Ui::Scenes::SceneId& nextSceneId);

protected:
  static const int IDA_BUTTON_OK = 101;
};
