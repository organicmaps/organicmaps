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

uint64_t const kDoubleTapPauseMs = 250;
uint64_t const kLongTouchMs = 1000;
uint64_t const kKineticDelayMs = 500;

double const kMaxAnimationTimeSec = 1.5; // in seconds

size_t GetValidTouchesCount(array<Touch, 2> const & touches)
{
  size_t result = 0;
  if (touches[0].m_id != -1)
    ++result;
  if (touches[1].m_id != -1)
    ++result;

  return result;
}

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

uint8_t const TouchEvent::INVALID_MASKED_POINTER = 0xFF;

void TouchEvent::PrepareTouches(array<Touch, 2> const & previousToches)
{
  if (GetValidTouchesCount(m_touches) == 2 && GetValidTouchesCount(previousToches) > 0)
  {
    if (previousToches[0].m_id == m_touches[1].m_id)
      Swap();
  }
}

void TouchEvent::SetFirstMaskedPointer(uint8_t firstMask)
{
  m_pointersMask = (m_pointersMask & 0xFF00) | static_cast<uint16_t>(firstMask);
}

uint8_t TouchEvent::GetFirstMaskedPointer() const
{
  return static_cast<uint8_t>(m_pointersMask & 0xFF);
}

void TouchEvent::SetSecondMaskedPointer(uint8_t secondMask)
{
  ASSERT(secondMask == INVALID_MASKED_POINTER || GetFirstMaskedPointer() != INVALID_MASKED_POINTER, ());
  m_pointersMask = (static_cast<uint16_t>(secondMask) << 8) | (m_pointersMask & 0xFF);
}

uint8_t TouchEvent::GetSecondMaskedPointer() const
{
  return static_cast<uint8_t>((m_pointersMask & 0xFF00) >> 8);
}

size_t TouchEvent::GetMaskedCount()
{
  return static_cast<int>(GetFirstMaskedPointer() != INVALID_MASKED_POINTER) +
         static_cast<int>(GetSecondMaskedPointer() != INVALID_MASKED_POINTER);
}

void TouchEvent::Swap()
{
  auto swapIndex = [](uint8_t index) -> uint8_t
  {
    if (index == INVALID_MASKED_POINTER)
      return index;

    return index ^ 0x1;
  };

  swap(m_touches[0], m_touches[1]);
  SetFirstMaskedPointer(swapIndex(GetFirstMaskedPointer()));
  SetSecondMaskedPointer(swapIndex(GetSecondMaskedPointer()));
}

UserEventStream::UserEventStream(TIsCountryLoaded const & fn)
  : m_isCountryLoaded(fn)
  , m_state(STATE_EMPTY)
  , m_startDragOrg(m2::PointD::Zero())
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
  bool breakAnim = false;
  for (UserEvent const & e : events)
  {
    switch (e.m_type)
    {
    case UserEvent::EVENT_SCALE:
      breakAnim = SetScale(e.m_scaleEvent.m_pxPoint, e.m_scaleEvent.m_factor, e.m_scaleEvent.m_isAnim);
      TouchCancel(m_touches, 0.0);
      break;
    case UserEvent::EVENT_RESIZE:
      m_navigator.OnSize(e.m_resize.m_width, e.m_resize.m_height);
      viewportChanged = true;
      breakAnim = true;
      TouchCancel(m_touches, 0.0);
      break;
    case UserEvent::EVENT_SET_ANY_RECT:
      breakAnim = SetRect(e.m_anyRect.m_rect, e.m_anyRect.m_isAnim);
      TouchCancel(m_touches, 0.0);
      break;
    case UserEvent::EVENT_SET_RECT:
      breakAnim = SetRect(e.m_rectEvent.m_rect, e.m_rectEvent.m_zoom, e.m_rectEvent.m_applyRotation, e.m_rectEvent.m_isAnim);
      TouchCancel(m_touches, 0.0);
      break;
    case UserEvent::EVENT_SET_CENTER:
      breakAnim = SetCenter(e.m_centerEvent.m_center, e.m_centerEvent.m_zoom, e.m_centerEvent.m_isAnim);
      TouchCancel(m_touches, 0.0);
      break;
    case UserEvent::EVENT_TOUCH:
      breakAnim = ProcessTouch(e.m_touchEvent);
      break;
    case UserEvent::EVENT_ROTATE:
      {
        m2::AnyRectD dstRect = GetTargetRect();
        dstRect.SetAngle(e.m_rotate.m_targetAzimut);
        breakAnim = SetRect(dstRect, true);
      }
      break;
    case UserEvent::EVENT_FOLLOW_AND_ROTATE:
      breakAnim = SetFollowAndRotate(e.m_followAndRotate.m_targetRect, e.m_followAndRotate.m_userPos,
                                     e.m_followAndRotate.m_newCenterOffset,e.m_followAndRotate.m_oldCenterOffset,
                                     e.m_followAndRotate.m_azimuth, e.m_followAndRotate.m_isAnim);
      TouchCancel(m_touches, 0.0);
      break;
    default:
      ASSERT(false, ());
      break;
    }
  }

  if (breakAnim)
    m_animation.reset();

  if (m_animation != nullptr)
  {
    m2::AnyRectD rect = m_animation->GetCurrentRect(GetCurrentScreen());
    m_navigator.SetFromRect(rect);
    modelViewChange = true;
    if (m_animation->IsFinished())
      m_animation.reset();
  }

  if (GetValidTouchesCount(m_touches) == 1)
  {
    if (m_state == STATE_WAIT_DOUBLE_TAP)
      DetectShortTap(m_touches[0]);
    else if (m_state == STATE_TAP_DETECTION)
      DetectLongTap(m_touches[0]);
  }

  return m_navigator.Screen();
}

ScreenBase const & UserEventStream::GetCurrentScreen() const
{
  return m_navigator.Screen();
}

bool UserEventStream::SetScale(m2::PointD const & pxScaleCenter, double factor, bool isAnim)
{
  m2::PointD scaleCenter = pxScaleCenter;
  if (m_listener)
    m_listener->CorrectScalePoint(scaleCenter);

  if (isAnim)
  {
    m2::AnyRectD rect = GetTargetRect();
    m2::PointD currentCenter = rect.GlobalZero();
    m2::PointD const glbScaleCenter = m_navigator.PtoG(scaleCenter);
    m2::PointD const centerMove = (currentCenter - glbScaleCenter) / factor;

    currentCenter = glbScaleCenter + centerMove;

    m2::RectD sizeRect = rect.GetLocalRect();
    sizeRect.Scale(1.0 / factor);

    auto creator = [this, &scaleCenter, &glbScaleCenter](m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                                                         double aDuration, double mDuration, double sDuration)
    {
      m_animation.reset(new FixedPointAnimation(startRect, endRect, aDuration, mDuration,
                                                sDuration, scaleCenter, glbScaleCenter));
    };

    return SetRect(m2::AnyRectD(currentCenter, rect.Angle(), sizeRect), true, creator);
  }

  m_navigator.Scale(scaleCenter, factor);
  return true;
}

bool UserEventStream::SetCenter(m2::PointD const & center, int zoom, bool isAnim)
{
  if (zoom == -1)
  {
    m2::AnyRectD r = GetTargetRect();
    return SetRect(m2::AnyRectD(center, r.Angle(), r.GetLocalRect()), isAnim);
  }

  return SetRect(df::GetRectForDrawScale(zoom, center), zoom, true, isAnim);
}

bool UserEventStream::SetRect(m2::RectD rect, int zoom, bool applyRotation, bool isAnim)
{
  CheckMinGlobalRect(rect);
  CheckMinMaxVisibleScale(m_isCountryLoaded, rect, zoom);
  m2::AnyRectD targetRect = applyRotation ? ToRotated(m_navigator, rect) : m2::AnyRectD(rect);
  return SetRect(targetRect, isAnim);
}

bool UserEventStream::SetRect(m2::AnyRectD const & rect, bool isAnim)
{
  return SetRect(rect, isAnim, [this](m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                                      double aDuration, double mDuration, double sDuration)
  {
    m_animation.reset(new ModelViewAnimation(startRect, endRect, aDuration, mDuration, sDuration));
  });
}

bool UserEventStream::SetRect(m2::AnyRectD const & rect, bool isAnim, TAnimationCreator const & animCreator)
{
  if (isAnim)
  {
    ScreenBase const & screen = m_navigator.Screen();
    m2::AnyRectD const startRect = GetCurrentRect();
    double const angleDuration = ModelViewAnimation::GetRotateDuration(startRect.Angle().val(), rect.Angle().val());
    double const moveDuration = ModelViewAnimation::GetMoveDuration(startRect.GlobalZero(), rect.GlobalZero(), screen);
    double const scaleDuration = ModelViewAnimation::GetScaleDuration(startRect.GetLocalRect().SizeX(),
                                                                      rect.GetLocalRect().SizeX());
    if (max(max(angleDuration, moveDuration), scaleDuration) < kMaxAnimationTimeSec)
    {
      ASSERT(animCreator != nullptr, ());
      animCreator(startRect, rect, angleDuration, moveDuration, scaleDuration);
      return false;
    }
  }

  m_animation.reset();
  m_navigator.SetFromRect(rect);
  return true;
}

bool UserEventStream::SetFollowAndRotate(m2::AnyRectD const & rect, m2::PointD const & userPos,
                                         double newCenterOffset, double oldCenterOffset, double azimuth, bool isAnim)
{
  if (isAnim)
  {
    ScreenBase const & screen = m_navigator.Screen();
    m2::AnyRectD const startRect = GetCurrentRect();
    double const angleDuration = ModelViewAnimation::GetRotateDuration(startRect.Angle().val(), azimuth);
    double const moveDuration = ModelViewAnimation::GetMoveDuration(startRect.GlobalZero(), rect.GlobalZero(), screen);
    double const duration = max(angleDuration, moveDuration);
    if (duration > 0.0 && duration < kMaxAnimationTimeSec)
    {
      m_animation.reset(new FollowAndRotateAnimation(startRect, userPos, newCenterOffset, oldCenterOffset, azimuth, duration));
      return false;
    }
  }

  m_animation.reset();
  m_navigator.SetFromRect(rect);
  return true;
}

m2::AnyRectD UserEventStream::GetCurrentRect() const
{
  return m_navigator.Screen().GlobalRect();
}

m2::AnyRectD UserEventStream::GetTargetRect() const
{
  if (m_animation)
    return m_animation->GetTargetRect(GetCurrentScreen());
  else
    return GetCurrentRect();
}

bool UserEventStream::ProcessTouch(TouchEvent const & touch)
{
  ASSERT(touch.m_touches[0].m_id != -1, ());

  TouchEvent touchEvent = touch;
  touchEvent.PrepareTouches(m_touches);
  bool isMapTouch = false;

  switch (touchEvent.m_type)
  {
  case TouchEvent::TOUCH_DOWN:
    isMapTouch |= TouchDown(touchEvent.m_touches, touch.m_timeStamp);
    break;
  case TouchEvent::TOUCH_MOVE:
    isMapTouch |= TouchMove(touchEvent.m_touches, touch.m_timeStamp);
    break;
  case TouchEvent::TOUCH_CANCEL:
    isMapTouch |= TouchCancel(touchEvent.m_touches, touch.m_timeStamp);
    break;
  case TouchEvent::TOUCH_UP:
    isMapTouch |= TouchUp(touchEvent.m_touches, touch.m_timeStamp);
    break;
  default:
    ASSERT(false, ());
    break;
  }

  return isMapTouch;
}

bool UserEventStream::TouchDown(array<Touch, 2> const & touches, double timestamp)
{
  size_t touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;

  if (touchCount == 1)
  {
    if (!DetectDoubleTap(touches[0]))
    {
      if (m_state == STATE_EMPTY)
      {
        if (!TryBeginFilter(touches[0]))
        {
          BeginTapDetector();
          m_startDragOrg = touches[0].m_location;
        }
        else
          isMapTouch = false;
      }
    }
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
    case STATE_WAIT_DOUBLE_TAP:
      CancelTapDetector();
      break;
    case STATE_DRAG:
      isMapTouch = EndDrag(touches[0], timestamp, true /* cancelled */);
      break;
    default:
      break;
    }

    BeginScale(touches[0], touches[1]);
  }

  UpdateTouches(touches);
  return isMapTouch;
}

bool UserEventStream::TouchMove(array<Touch, 2> const & touches, double timestamp)
{
  size_t touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;

  switch (m_state)
  {
  case STATE_EMPTY:
    if (touchCount == 1)
      BeginDrag(touches[0], timestamp);
    else
      BeginScale(touches[0], touches[1]);
    break;
  case STATE_FILTER:
    ASSERT_EQUAL(touchCount, 1, ());
    isMapTouch = false;
    break;
  case STATE_TAP_DETECTION:
  case STATE_WAIT_DOUBLE_TAP:
    ASSERT_EQUAL(touchCount, 1, ());
    if (m_startDragOrg.SquareLength(touches[0].m_location) > VisualParams::Instance().GetDragThreshold())
      CancelTapDetector();
    break;
  case STATE_DRAG:
    ASSERT_EQUAL(touchCount, 1, ());
    Drag(touches[0], timestamp);
    break;
  case STATE_SCALE:
    ASSERT_EQUAL(touchCount, 2, ());
    Scale(touches[0], touches[1]);
    break;
  default:
    ASSERT(false, ());
    break;
  }

  UpdateTouches(touches);
  return isMapTouch;
}

bool UserEventStream::TouchCancel(array<Touch, 2> const & touches, double timestamp)
{
  size_t touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;
  switch (m_state)
  {
  case STATE_EMPTY:
  case STATE_WAIT_DOUBLE_TAP:
    isMapTouch = false;
    break;
  case STATE_FILTER:
    ASSERT_EQUAL(touchCount, 1, ());
    CancelFilter(touches[0]);
    break;
  case STATE_TAP_DETECTION:
    ASSERT_EQUAL(touchCount, 1, ());
    CancelTapDetector();
    isMapTouch = false;
    break;
  case STATE_DRAG:
    ASSERT_EQUAL(touchCount, 1, ());
    isMapTouch = EndDrag(touches[0], timestamp, true /* cancelled */);
    break;
  case STATE_SCALE:
    ASSERT_EQUAL(touchCount, 2, ());
    EndScale(touches[0], touches[1]);
    break;
  default:
    ASSERT(false, ());
    break;
  }
  UpdateTouches(touches);
  return isMapTouch;
}

bool UserEventStream::TouchUp(array<Touch, 2> const & touches, double timestamp)
{
  size_t touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;
  switch (m_state)
  {
  case STATE_EMPTY:
    isMapTouch = false;
    // Can be if long tap or double tap detected
    break;
  case STATE_FILTER:
    ASSERT_EQUAL(touchCount, 1, ());
    EndFilter(touches[0]);
    isMapTouch = false;
    break;
  case STATE_TAP_DETECTION:
    ASSERT_EQUAL(touchCount, 1, ());
    EndTapDetector(touches[0]);
    break;
  case STATE_DRAG:
    ASSERT_EQUAL(touchCount, 1, ());
    isMapTouch = EndDrag(touches[0], timestamp, false /* cancelled */);
    break;
  case STATE_SCALE:
    ASSERT_EQUAL(touchCount, 2, ());
    EndScale(touches[0], touches[1]);
    break;
  default:
    ASSERT(false, ());
    break;
  }

  UpdateTouches(touches);
  return isMapTouch;
}

void UserEventStream::UpdateTouches(array<Touch, 2> const & touches)
{
  m_touches = touches;
}

void UserEventStream::BeginDrag(Touch const & t, double timestamp)
{
  TEST_CALL(BEGIN_DRAG);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  m_state = STATE_DRAG;
  m_startDragOrg = m_navigator.Screen().GetOrg();
  if (m_listener)
    m_listener->OnDragStarted();
  m_navigator.StartDrag(t.m_location);

  if (!m_scroller.IsActive())
    m_scroller.InitGrab(m_navigator.Screen(), timestamp);
}

void UserEventStream::Drag(Touch const & t, double timestamp)
{
  TEST_CALL(DRAG);
  ASSERT_EQUAL(m_state, STATE_DRAG, ());
  m_navigator.DoDrag(t.m_location);

  if (m_scroller.IsActive())
    m_scroller.GrabViewRect(m_navigator.Screen(), timestamp);
}

bool UserEventStream::EndDrag(Touch const & t, double timestamp, bool cancelled)
{
  TEST_CALL(END_DRAG);
  ASSERT_EQUAL(m_state, STATE_DRAG, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnDragEnded(m_navigator.GtoP(m_navigator.Screen().GetOrg()) - m_navigator.GtoP(m_startDragOrg));

  m_startDragOrg = m2::PointD::Zero();
  m_navigator.StopDrag(t.m_location);

  if (cancelled)
  {
    m_scroller.CancelGrab();
    return true;
  }

  if (m_kineticTimer.TimeElapsedAs<milliseconds>().count() >= kKineticDelayMs)
  {
    m_scroller.GrabViewRect(m_navigator.Screen(), timestamp);
    m_animation = m_scroller.CreateKineticAnimation(m_navigator.Screen());
    m_scroller.CancelGrab();
    return false;
  }

  return true;
}

void UserEventStream::BeginScale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(BEGIN_SCALE);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  m_state = STATE_SCALE;
  m2::PointD touch1 = t1.m_location;
  m2::PointD touch2 = t2.m_location;

  if (m_listener)
  {
    m_listener->OnScaleStarted();
    m_listener->CorrectScalePoint(touch1, touch2);
  }

  m_navigator.StartScale(touch1, touch2);
}

void UserEventStream::Scale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(SCALE);
  ASSERT_EQUAL(m_state, STATE_SCALE, ());

  m2::PointD touch1 = t1.m_location;
  m2::PointD touch2 = t2.m_location;

  if (m_listener)
  {
    if (m_navigator.IsRotatingDuringScale())
      m_listener->OnRotated();

    m_listener->CorrectScalePoint(touch1, touch2);
  }

  m_navigator.DoScale(touch1, touch2);
}

void UserEventStream::EndScale(const Touch & t1, const Touch & t2)
{
  TEST_CALL(END_SCALE);
  ASSERT_EQUAL(m_state, STATE_SCALE, ());
  m_state = STATE_EMPTY;

  m2::PointD touch1 = t1.m_location;
  m2::PointD touch2 = t2.m_location;

  if (m_listener)
  {
    m_listener->CorrectScalePoint(touch1, touch2);
    m_listener->OnScaleEnded();
  }

  m_navigator.StopScale(touch1, touch2);

  m_kineticTimer.Reset();
}

void UserEventStream::BeginTapDetector()
{
  TEST_CALL(BEGIN_TAP_DETECTOR);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  m_state = STATE_TAP_DETECTION;
  m_touchTimer.Reset();
}

void UserEventStream::DetectShortTap(Touch const & touch)
{
  if (m_touchTimer.ElapsedMillis() > kDoubleTapPauseMs)
  {
    m_state = STATE_EMPTY;
    if (m_listener)
      m_listener->OnTap(touch.m_location, false);
  }
}

void UserEventStream::DetectLongTap(Touch const & touch)
{
  ASSERT_EQUAL(m_state, STATE_TAP_DETECTION, ());
  if (m_touchTimer.ElapsedMillis() > kLongTouchMs)
  {
    TEST_CALL(LONG_TAP_DETECTED);
    m_state = STATE_EMPTY;
    if (m_listener)
      m_listener->OnTap(touch.m_location, true);
  }
}

bool UserEventStream::DetectDoubleTap(Touch const & touch)
{
  if (m_state != STATE_WAIT_DOUBLE_TAP || m_touchTimer.ElapsedMillis() > kDoubleTapPauseMs)
    return false;

  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnDoubleTap(touch.m_location);

  return true;
}

void UserEventStream::EndTapDetector(Touch const & touch)
{
  TEST_CALL(SHORT_TAP_DETECTED);
  ASSERT_EQUAL(m_state, STATE_TAP_DETECTION, ());
  m_state = STATE_WAIT_DOUBLE_TAP;
}

void UserEventStream::CancelTapDetector()
{
  TEST_CALL(CANCEL_TAP_DETECTOR);
  ASSERT(m_state == STATE_TAP_DETECTION || m_state == STATE_WAIT_DOUBLE_TAP, ());
  m_state = STATE_EMPTY;
}

bool UserEventStream::TryBeginFilter(Touch const & t)
{
  TEST_CALL(TRY_FILTER);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  if (m_listener && m_listener->OnSingleTouchFiltrate(t.m_location, TouchEvent::TOUCH_DOWN))
  {
    m_state = STATE_FILTER;
    return true;
  }

  return false;
}

void UserEventStream::EndFilter(const Touch & t)
{
  TEST_CALL(END_FILTER);
  ASSERT_EQUAL(m_state, STATE_FILTER, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnSingleTouchFiltrate(t.m_location, TouchEvent::TOUCH_UP);
}

void UserEventStream::CancelFilter(Touch const & t)
{
  TEST_CALL(CANCEL_FILTER);
  ASSERT_EQUAL(m_state, STATE_FILTER, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnSingleTouchFiltrate(t.m_location, TouchEvent::TOUCH_CANCEL);
}

bool UserEventStream::IsInUserAction() const
{
  return m_state == STATE_DRAG || m_state == STATE_SCALE;
}

}
