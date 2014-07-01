#pragma once

#include <FUiITouchEventListener.h>

#include "../../../std/utility.hpp"
#include "../../../std/vector.hpp"
#include "../../../map/user_mark.hpp"

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

  typedef vector<m2::PointD> TPointPairs;
private:

  void StartMove(TPointPairs const & pts);
  enum EState
  {
    ST_WAIT_TIMER,
    ST_MOVING,
    ST_ROTATING,
    ST_EMPTY
  };

  EState m_state;
  MapsWithMeForm * m_pForm;
  bool m_wasLongPress;
  bool m_bWasReleased;
  m2::PointD m_startTouchPoint;
  TPointPairs m_prev_pts;
  Tizen::Base::Runtime::Timer m_timer;
};
