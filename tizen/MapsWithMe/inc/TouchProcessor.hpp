#pragma once

#include <FUiITouchEventListener.h>

#include "../../../std/utility.hpp"
#include "../../../std/vector.hpp"

class MapsWithMeForm;

class TouchProcessor: public Tizen::Ui::ITouchEventListener
  , public Tizen::Base::Runtime::ITimerEventListener
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
  // ITimerEventListener
  virtual void  OnTimerExpired (Tizen::Base::Runtime::Timer & timer);

  typedef vector<pair<double, double> > TPointPairs;
private:

  void StartMove(TPointPairs const & pts);
  enum EState
  {
    st_waitTimer,
    st_moving,
    st_rotating,
    st_empty
  };

  EState m_state;
  MapsWithMeForm * m_pForm;
  bool m_wasLongPress;
  bool m_bWasReleased;
  pair<double, double> m_startTouchPoint;
  vector<pair<double, double> > m_prev_pts;
  Tizen::Base::Runtime::Timer m_timer;
};
