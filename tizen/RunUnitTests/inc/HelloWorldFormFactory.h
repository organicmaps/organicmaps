#pragma once
#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

class HelloWorldFormFactory: public Tizen::Ui::Scenes::IFormFactory
{
public:
  HelloWorldFormFactory(void);
  virtual ~HelloWorldFormFactory(void);

  virtual Tizen::Ui::Controls::Form* CreateFormN(const Tizen::Base::String& formId,
      const Tizen::Ui::Scenes::SceneId& sceneId);
};
