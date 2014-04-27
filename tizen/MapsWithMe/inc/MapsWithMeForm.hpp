#pragma once

#include <FUi.h>
#include <FUiITouchEventListener.h>
#include "../../../std/vector.hpp"

class MapsWithMeApp;

class MapsWithMeForm
: public Tizen::Ui::Controls::Form
  , public Tizen::Ui::ITouchEventListener
  , public Tizen::Ui::IActionEventListener
{
public:
  MapsWithMeForm(MapsWithMeApp* pApp);
  virtual ~MapsWithMeForm(void);

  virtual result OnDraw(void);
  bool Initialize(void);
  virtual result OnInitializing(void);

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

  virtual void OnActionPerformed(const Tizen::Ui::Control& source, int actionId);
private:
  std::vector<std::pair<double, double> > m_prev_pts;
  MapsWithMeApp* m_pApp;
};
