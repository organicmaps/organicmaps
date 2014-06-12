#pragma once

#include <FUi.h>

class SelectColorForm: public Tizen::Ui::Controls::Form
, public Tizen::Ui::Controls::IFormBackEventListener
, public Tizen::Ui::IActionEventListener
{
public:
  SelectColorForm();
  virtual ~SelectColorForm(void);

  bool Initialize(void);
  virtual result OnInitializing(void);
  virtual void OnFormBackRequested(Tizen::Ui::Controls::Form & source);
  virtual void OnActionPerformed(Tizen::Ui::Control const & source, int actionId);

private:
  enum EActionId
  {
    ID_RED,
    ID_BLUE,
    ID_BROWN,
    ID_GREEN,
    ID_ORANGE,
    ID_PINK,
    ID_PURPLE,
    ID_YELLOW
  };
};
