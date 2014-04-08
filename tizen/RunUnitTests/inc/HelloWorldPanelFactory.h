#pragma once
#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

class HelloWorldPanelFactory: public Tizen::Ui::Scenes::IPanelFactory
{
public:
  HelloWorldPanelFactory(void);
  virtual ~HelloWorldPanelFactory(void);

  virtual Tizen::Ui::Controls::Panel* CreatePanelN(const Tizen::Base::String& panelId,
      const Tizen::Ui::Scenes::SceneId& sceneId);
};
