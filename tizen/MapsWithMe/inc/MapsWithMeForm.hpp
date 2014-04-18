#pragma once

#include <FUi.h>
#include <FUiITouchEventListener.h>

class MapsWithMeApp;

class MapsWithMeForm
: public Tizen::Ui::Controls::Form
  , public Tizen::Ui::ITouchEventListener
  {
  public:
  MapsWithMeForm(MapsWithMeApp* pApp);
  virtual ~MapsWithMeForm(void);

  virtual result OnDraw(void);

  //  virtual void  OnTouchCanceled (const Tizen::Ui::Control &source, const Tizen::Graphics::Point &currentPosition, const Tizen::Ui::TouchEventInfo &touchInfo)
  virtual void  OnTouchFocusIn (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchFocusOut (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchMoved (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchPressed (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  virtual void  OnTouchReleased (const Tizen::Ui::Control &source,
      const Tizen::Graphics::Point &currentPosition,
      const Tizen::Ui::TouchEventInfo &touchInfo);
  private:
  MapsWithMeApp* m_pApp;
  };
