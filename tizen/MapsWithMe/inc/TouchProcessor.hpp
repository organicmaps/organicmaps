#pragma once

#include <FUiITouchEventListener.h>
#include "../../../std/utility.hpp"
#include "../../../std/vector.hpp"

class MapsWithMeForm;

class TouchProcessor: public Tizen::Ui::ITouchEventListener
{
public:
  TouchProcessor(MapsWithMeForm * pForm);
  // ITouchEventListener
  virtual void  OnTouchFocusIn (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo){}
  virtual void  OnTouchFocusOut (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo){}
  virtual void  OnTouchMoved (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchPressed (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void  OnTouchReleased (Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void OnTouchLongPressed(Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
  virtual void OnTouchDoublePressed(Tizen::Ui::Control const & source,
      Tizen::Graphics::Point const & currentPosition,
      Tizen::Ui::TouchEventInfo const & touchInfo);
private:

  MapsWithMeForm * m_pForm;

  bool m_wasLongPress;
  pair<double, double> m_startTouchPoint;
  vector<pair<double, double> > m_prev_pts;
};
