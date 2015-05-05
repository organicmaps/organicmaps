#pragma once

#include "drape_frontend/navigator.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

#include "base/timer.hpp"

#include "std/array.hpp"
#include "std/bitset.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/list.hpp"

namespace df
{

struct Touch
{
  m2::PointF m_location = m2::PointF::Zero();
  int m_id = -1; // if id == -1 than touch invalid
};

struct TouchEvent
{
  TouchEvent()
    : m_type(TOUCH_CANCEL)
  {
  }

  enum ETouchType
  {
    TOUCH_DOWN,
    TOUCH_MOVE,
    TOUCH_UP,
    TOUCH_CANCEL
  };

  ETouchType m_type;
  array<Touch, 2> m_touches;
};

struct ScaleEvent
{
  ScaleEvent(double factor, m2::PointD const & pxPoint)
    : m_factor(factor)
    , m_pxPoint(pxPoint)
  {
  }

  double m_factor;
  m2::PointD m_pxPoint;
};

struct SetCenterEvent
{
  SetCenterEvent(m2::PointD const & center, int zoom)
    : m_center(center)
    , m_zoom(zoom)
  {
  }

  m2::PointD m_center; // center point in mercator
  int m_zoom; // if zoom == -1, then zoom level will'n change
};

struct SetRectEvent
{
  SetRectEvent(m2::RectD const & rect, bool rotate, int zoom)
    : m_rect(rect)
    , m_applyRotation(rotate)
    , m_zoom(zoom)
  {
  }

  m2::RectD m_rect; // destination mercator rect
  bool m_applyRotation; // if true, current rotation will be apply to m_rect
  int m_zoom; // if zoom == -1, then zoom level will'n change
};

struct SetAnyRectEvent
{
  SetAnyRectEvent(m2::AnyRectD const & rect) : m_rect(rect) {}

  m2::AnyRectD m_rect;  // destination mercator rect
};

struct ResizeEvent
{
  ResizeEvent(uint32_t w, uint32_t h) : m_width(w), m_height(h) {}

  uint32_t m_width;
  uint32_t m_height;
};

struct UserEvent
{
  enum EEventType
  {
    EVENT_TOUCH,
    EVENT_SCALE,
    EVENT_SET_CENTER,
    EVENT_SET_RECT,
    EVENT_SET_ANY_RECT,
    EVENT_RESIZE
  };

  UserEvent(TouchEvent const & e) : m_type(EVENT_TOUCH) { m_touchEvent = e; }
  UserEvent(ScaleEvent const & e) : m_type(EVENT_SCALE) { m_scaleEvent = e; }
  UserEvent(SetCenterEvent const & e) : m_type(EVENT_SET_CENTER) { m_centerEvent = e; }
  UserEvent(SetRectEvent const & e) : m_type(EVENT_SET_RECT) { m_rectEvent = e; }
  UserEvent(SetAnyRectEvent const & e) : m_type(EVENT_SET_ANY_RECT) { m_anyRect = e; }
  UserEvent(ResizeEvent const & e) : m_type(EVENT_RESIZE) { m_resize = e; }

  EEventType m_type;
  union
  {
    TouchEvent m_touchEvent;
    ScaleEvent m_scaleEvent;
    SetCenterEvent m_centerEvent;
    SetRectEvent m_rectEvent;
    SetAnyRectEvent m_anyRect;
    ResizeEvent m_resize;
  };
};

class UserEventStream
{
public:
  UserEventStream(TIsCountryLoaded const & fn);
  void AddEvent(UserEvent const & event);
  ScreenBase const & ProcessEvents(bool & modelViewChange, bool & viewportChanged);

  using TTapDetectedFn = function<void (m2::PointD const & pt, bool isLong)>;
  using TSingleTouchFilterFn = function<bool (m2::PointD const &, TouchEvent::ETouchType type)>;
  void SetTapListener(TTapDetectedFn const & tapCallback, TSingleTouchFilterFn const & filterFn);

private:
  void Clear();
  void SetCenter(m2::PointD const & center, int zoom);
  void SetRect(m2::RectD rect, int zoom, bool applyRotation);
  void SetRect(m2::AnyRectD const & rect);

  void ProcessTouch(TouchEvent const & touch);

  void TouchDown(array<Touch, 2> const  & touches);
  void TouchMove(array<Touch, 2> const  & touches);
  void TouchCancel(array<Touch, 2> const  & touches);
  void TouchUp(array<Touch, 2> const  & touches);
  void UpdateTouches(array<Touch, 2> const  & touches, size_t validCount);
  size_t GetValidTouchesCount(array<Touch, 2> const  & touches) const;

  void BeginDrag(Touch const & t);
  void Drag(Touch const & t);
  void EndDrag(Touch const & t);

  void BeginScale(Touch const & t1, Touch const & t2);
  void Scale(Touch const & t1, Touch const & t2);
  void EndScale(Touch const & t1, Touch const & t2);

  void BeginTapDetector();
  void DetectLongTap(Touch const & touch);
  void EndTapDetector(Touch const & touch);
  void CancelTapDetector();

  bool TryBeginFilter(Touch const & t);
  void EndFilter(Touch const & t);
  void CancelFilter(Touch const & t);

private:
  TIsCountryLoaded m_isCountryLoaded;
  TTapDetectedFn m_tapDetectedFn;
  TSingleTouchFilterFn m_filterFn;

  list<UserEvent> m_events;
  mutable mutex m_lock;

  Navigator m_navigator;
  my::HighResTimer m_touchTimer;
  enum ERecognitionState
  {
    STATE_EMPTY,
    STATE_FILTER,
    STATE_TAP_DETECTION,
    STATE_DRAG,
    STATE_SCALE
  } m_state;

  array<Touch, 2> m_touches;
  size_t m_validPointersCount;
};

}
