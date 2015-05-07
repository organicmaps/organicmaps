#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#ifdef DEBUG
#define TEST_CALL(action) if (m_testFn) m_testFn(action)
#else
#define TEST_CALL(action)
#endif

namespace df
{

namespace
{

uint64_t const LONG_TOUCH_MS = 1000;

} // namespace

#ifdef DEBUG
char const * UserEventStream::BEGIN_DRAG = "BeginDrag";
char const * UserEventStream::DRAG = "Drag";
char const * UserEventStream::END_DRAG = "EndDrag";
char const * UserEventStream::BEGIN_SCALE = "BeginScale";
char const * UserEventStream::SCALE = "Scale";
char const * UserEventStream::END_SCALE = "EndScale";
char const * UserEventStream::BEGIN_TAP_DETECTOR = "BeginTap";
char const * UserEventStream::LONG_TAP_DETECTED = "LongTap";
char const * UserEventStream::SHORT_TAP_DETECTED = "ShortTap";
char const * UserEventStream::CANCEL_TAP_DETECTOR = "CancelTap";
char const * UserEventStream::TRY_FILTER = "TryFilter";
char const * UserEventStream::END_FILTER = "EndFilter";
char const * UserEventStream::CANCEL_FILTER = "CancelFilter";
#endif

UserEventStream::UserEventStream(TIsCountryLoaded const & fn)
  : m_isCountryLoaded(fn)
  , m_state(STATE_EMPTY)
{
}

void UserEventStream::AddEvent(UserEvent const & event)
{
  lock_guard<mutex> guard(m_lock);
  UNUSED_VALUE(guard);
  m_events.push_back(event);
}

ScreenBase const & UserEventStream::ProcessEvents(bool & modelViewChange, bool & viewportChanged)
{
  modelViewChange = false;
  viewportChanged = false;

  list<UserEvent> events;

  {
    lock_guard<mutex> guard(m_lock);
    UNUSED_VALUE(guard);
    swap(m_events, events);
  }

  modelViewChange = !events.empty() || m_state != STATE_EMPTY;
  for (UserEvent const & e : events)
  {
    switch (e.m_type)
    {
    case UserEvent::EVENT_SCALE:
      m_navigator.Scale(e.m_scaleEvent.m_pxPoint, e.m_scaleEvent.m_factor);
      TouchCancel(m_touches);
      break;
    case UserEvent::EVENT_RESIZE:
      m_navigator.OnSize(e.m_resize.m_width, e.m_resize.m_height);
      viewportChanged = true;
      TouchCancel(m_touches);
      break;
    case UserEvent::EVENT_SET_ANY_RECT:
      SetRect(e.m_anyRect.m_rect);
      TouchCancel(m_touches);
      break;
    case UserEvent::EVENT_SET_RECT:
      SetRect(e.m_rectEvent.m_rect, e.m_rectEvent.m_zoom, e.m_rectEvent.m_applyRotation);
      TouchCancel(m_touches);
      break;
    case UserEvent::EVENT_SET_CENTER:
      SetCenter(e.m_centerEvent.m_center, e.m_centerEvent.m_zoom);
      TouchCancel(m_touches);
      break;
    case UserEvent::EVENT_TOUCH:
      ProcessTouch(e.m_touchEvent);
      break;
    default:
      ASSERT(false, ());
      break;
    }
  }

  if (m_state == STATE_TAP_DETECTION && m_validPointersCount == 1)
    DetectLongTap(m_touches[0]);

  return m_navigator.Screen();
}

void UserEventStream::SetTapListener(TTapDetectedFn const & tapCallback, TSingleTouchFilterFn const & filterFn)
{
  m_tapDetectedFn = tapCallback;
  m_filterFn = filterFn;
}

void UserEventStream::Clear()
{
  m_events.clear();
}

void UserEventStream::SetCenter(m2::PointD const & center, int zoom)
{
  m2::RectD rect = df::GetRectForDrawScale(zoom, center);
  SetRect(rect, zoom, true);
}

void UserEventStream::SetRect(m2::RectD rect, int zoom, bool applyRotation)
{
  CheckMinGlobalRect(rect);
  CheckMinMaxVisibleScale(m_isCountryLoaded, rect, zoom);
  if (applyRotation)
    SetRect(ToRotated(m_navigator, rect));
  else
    SetRect(m2::AnyRectD(rect));
}

void UserEventStream::SetRect(m2::AnyRectD const & rect)
{
  double const halfSize = df::VisualParams::Instance().GetTileSize() / 2.0;
  m2::RectD etalonRect(-halfSize, -halfSize, halfSize, halfSize);

  m2::PointD const pxCenter = m_navigator.Screen().PixelRect().Center();
  etalonRect.Offset(pxCenter);

  m_navigator.SetFromRects(rect, etalonRect);
}

void UserEventStream::ProcessTouch(TouchEvent const & touch)
{
  ASSERT(touch.m_touches[0].m_id  != -1, ());
  ASSERT(touch.m_touches[1].m_id == -1 ||
         (touch.m_touches[0].m_id < touch.m_touches[1].m_id), ());

  switch (touch.m_type)
  {
  case TouchEvent::TOUCH_DOWN:
    TouchDown(touch.m_touches);
    break;
  case TouchEvent::TOUCH_MOVE:
    TouchMove(touch.m_touches);
    break;
  case TouchEvent::TOUCH_CANCEL:
    TouchCancel(touch.m_touches);
    break;
  case TouchEvent::TOUCH_UP:
    TouchUp(touch.m_touches);
    break;
  default:
    ASSERT(false, ());
    break;
  }
}

void UserEventStream::TouchDown(array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);

  if (touchCount == 1)
  {
    ASSERT(m_state == STATE_EMPTY, ());
    if (!TryBeginFilter(touches[0]))
      BeginTapDetector();
  }
  else if (touchCount == 2)
  {
    switch (m_state)
    {
    case STATE_EMPTY:
      break;
    case STATE_FILTER:
      CancelFilter(touches[0]);
      break;
    case STATE_TAP_DETECTION:
      CancelTapDetector();
      break;
    case STATE_DRAG:
      EndDrag(touches[0]);
      break;
    default:
      ASSERT(false, ());
      break;
    }

    BeginScale(touches[0], touches[1]);
  }

  UpdateTouches(touches, touchCount);
}

void UserEventStream::TouchMove(array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);

  switch (m_state)
  {
  case STATE_EMPTY:
    if (touchCount == 1)
      BeginDrag(touches[0]);
    else
      BeginScale(touches[0], touches[1]);
    break;
  case STATE_FILTER:
    ASSERT(touchCount == 1, ());
    break;
  case STATE_TAP_DETECTION:
    ASSERT(touchCount == 1, ());
    CancelTapDetector();
    break;
  case STATE_DRAG:
    ASSERT(touchCount == 1, ());
    Drag(touches[0]);
    break;
  case STATE_SCALE:
    ASSERT(touchCount == 2, ());
    Scale(touches[0], touches[1]);
    break;
  default:
    break;
  }

  UpdateTouches(touches, touchCount);
}

void UserEventStream::TouchCancel(array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);
  switch (m_state)
  {
  case STATE_EMPTY:
    break;
  case STATE_FILTER:
    ASSERT(touchCount == 1, ());
    CancelFilter(touches[0]);
    break;
  case STATE_TAP_DETECTION:
    ASSERT(touchCount == 1, ());
    CancelTapDetector();
    break;
  case STATE_DRAG:
    ASSERT(touchCount == 1, ());
    EndDrag(touches[0]);
    break;
  case STATE_SCALE:
    ASSERT(touchCount == 2, ());
    EndScale(touches[0], touches[1]);
    break;
  default:
    break;
  }
  UpdateTouches(touches, touchCount);
}

void UserEventStream::TouchUp(array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);
  switch (m_state)
  {
  case STATE_EMPTY:
    // Can be if long tap detected
    break;
  case STATE_FILTER:
    ASSERT(touchCount == 1, ());
    EndFilter(touches[0]);
    break;
  case STATE_TAP_DETECTION:
    ASSERT(touchCount == 1, ());
    EndTapDetector(touches[0]);
    break;
  case STATE_DRAG:
    ASSERT(touchCount == 1, ());
    EndDrag(touches[0]);
    break;
  case STATE_SCALE:
    ASSERT(touchCount == 2, ());
    EndScale(touches[0], touches[1]);
    break;
  default:
    break;
  }
  UpdateTouches(touches, touchCount);
}

void UserEventStream::UpdateTouches(array<Touch, 2> const & touches, size_t validCount)
{
  m_touches = touches;
  m_validPointersCount = validCount;
#ifdef DEBUG
  if (validCount > 0)
    ASSERT(m_touches[0].m_id != -1, ());
  if (validCount > 1)
    ASSERT(m_touches[1].m_id != -1, ());
#endif
}

size_t UserEventStream::GetValidTouchesCount(array<Touch, 2> const & touches) const
{
  size_t result = 0;
  if (touches[0].m_id != -1)
    ++result;
  if (touches[1].m_id != -1)
    ++result;

  return result;
}

void UserEventStream::BeginDrag(Touch const & t)
{
  TEST_CALL(BEGIN_DRAG);
  ASSERT(m_state == STATE_EMPTY, ());
  m_state = STATE_DRAG;
  m_navigator.StartDrag(t.m_location);
}

void UserEventStream::Drag(Touch const & t)
{
  TEST_CALL(DRAG);
  ASSERT(m_state == STATE_DRAG, ());
  m_navigator.DoDrag(t.m_location);
}

void UserEventStream::EndDrag(Touch const & t)
{
  TEST_CALL(END_DRAG);
  ASSERT(m_state == STATE_DRAG, ());
  m_state = STATE_EMPTY;
  m_navigator.StopDrag(t.m_location);
}

void UserEventStream::BeginScale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(BEGIN_SCALE);
  ASSERT(m_state == STATE_EMPTY, ());
  m_state = STATE_SCALE;
  m_navigator.StartScale(t1.m_location, t2.m_location);
}

void UserEventStream::Scale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(SCALE);
  ASSERT(m_state == STATE_SCALE, ());
  m_navigator.DoScale(t1.m_location, t2.m_location);
}

void UserEventStream::EndScale(const Touch & t1, const Touch & t2)
{
  TEST_CALL(END_SCALE);
  ASSERT(m_state == STATE_SCALE, ());
  m_state = STATE_EMPTY;
  m_navigator.StopScale(t1.m_location, t2.m_location);
}

void UserEventStream::BeginTapDetector()
{
  TEST_CALL(BEGIN_TAP_DETECTOR);
  ASSERT(m_state == STATE_EMPTY, ());
  m_state = STATE_TAP_DETECTION;
  m_touchTimer.Reset();
}

void UserEventStream::DetectLongTap(Touch const & touch)
{
  ASSERT(m_state == STATE_TAP_DETECTION, ());
  if (m_touchTimer.ElapsedMillis() > LONG_TOUCH_MS)
  {
    TEST_CALL(LONG_TAP_DETECTED);
    m_state = STATE_EMPTY;
    m_tapDetectedFn(touch.m_location, true);
  }
}

void UserEventStream::EndTapDetector(Touch const & touch)
{
  TEST_CALL(SHORT_TAP_DETECTED);
  ASSERT(m_state == STATE_TAP_DETECTION, ());
  m_state = STATE_EMPTY;
  m_tapDetectedFn(touch.m_location, false);
}

void UserEventStream::CancelTapDetector()
{
  TEST_CALL(CANCEL_TAP_DETECTOR);
  ASSERT(m_state == STATE_TAP_DETECTION, ());
  m_state = STATE_EMPTY;
}

bool UserEventStream::TryBeginFilter(Touch const & t)
{
  TEST_CALL(TRY_FILTER);
  ASSERT(m_state == STATE_EMPTY, ());
  if (m_filterFn(t.m_location, TouchEvent::TOUCH_DOWN))
  {
    m_state = STATE_FILTER;
    return true;
  }

  return false;
}

void UserEventStream::EndFilter(const Touch & t)
{
  TEST_CALL(END_FILTER);
  ASSERT(m_state == STATE_FILTER, ());
  m_state = STATE_EMPTY;
  m_filterFn(t.m_location, TouchEvent::TOUCH_UP);
}

void UserEventStream::CancelFilter(Touch const & t)
{
  TEST_CALL(CANCEL_FILTER);
  ASSERT(m_state == STATE_FILTER, ());
  m_state = STATE_EMPTY;
  m_filterFn(t.m_location, TouchEvent::TOUCH_CANCEL);
}

}
