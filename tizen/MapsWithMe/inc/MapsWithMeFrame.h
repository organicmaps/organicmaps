#pragma once

#include <FBase.h>
#include <FUi.h>
#include <FApp.h>

class MapsWithMeFrame
  : public Tizen::Ui::Controls::Frame
  , public Tizen::Ui::IPropagatedKeyEventListener
{
public:
  MapsWithMeFrame(void);
  virtual ~MapsWithMeFrame(void);

private:
  virtual result OnInitializing(void);
  virtual result OnTerminating(void);
  // key events for back-key
  virtual bool OnKeyPressed(Tizen::Ui::Control& source, const Tizen::Ui::KeyEventInfo& keyEventInfo) { return false; };
  virtual bool OnKeyReleased(Tizen::Ui::Control& source, const Tizen::Ui::KeyEventInfo& keyEventInfo);
  virtual bool OnPreviewKeyPressed(Tizen::Ui::Control& source, const Tizen::Ui::KeyEventInfo& keyEventInfo) { return false; };
  virtual bool OnPreviewKeyReleased(Tizen::Ui::Control& source, const Tizen::Ui::KeyEventInfo& keyEventInfo) { return false; };
  virtual bool TranslateKeyEventInfo(Tizen::Ui::Control& source, Tizen::Ui::KeyEventInfo& keyEventInfo) { return false; };
};

