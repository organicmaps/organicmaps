#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/screen_animations.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/macros.hpp"

#include <cmath>

#ifdef DEBUG
#define TEST_CALL(action) \
  if (m_testFn)           \
  m_testFn(action)
#else
#define TEST_CALL(action)
#endif

namespace df
{
namespace
{
uint64_t constexpr kDoubleTapPauseMs = 250;
uint64_t constexpr kLongTouchMs = 500;
uint64_t constexpr kKineticDelayMs = 500;

float constexpr kForceTapThreshold = 0.75;

bool IsLocalCameraMove(ScreenBase const & current, ScreenBase const & target)
{
  double constexpr kMaxZoomRatio = 4.0;
  double const zoomRatio = target.GetScale() / current.GetScale();
  return zoomRatio >= 1.0 / kMaxZoomRatio && zoomRatio <= kMaxZoomRatio &&
         IsWithinAnimationDistance(current, current.GetOrg(), target.GetOrg());
}

size_t GetValidTouchesCount(std::array<Touch, 2> const & touches)
{
  size_t result = 0;
  if (touches[0].m_id != -1)
    ++result;
  if (touches[1].m_id != -1)
    ++result;

  return result;
}
}  // namespace

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
char const * UserEventStream::TWO_FINGERS_TAP = "TwoFingersTap";
char const * UserEventStream::BEGIN_DOUBLE_TAP_AND_HOLD = "BeginDoubleTapAndHold";
char const * UserEventStream::DOUBLE_TAP_AND_HOLD = "DoubleTapAndHold";
char const * UserEventStream::END_DOUBLE_TAP_AND_HOLD = "EndDoubleTapAndHold";
#endif

void TouchEvent::SetFirstTouch(Touch const & touch)
{
  m_touches[0] = touch;
}

void TouchEvent::SetSecondTouch(Touch const & touch)
{
  m_touches[1] = touch;
}

void TouchEvent::PrepareTouches(std::array<Touch, 2> const & previousTouches)
{
  if (GetValidTouchesCount(m_touches) == 2 && GetValidTouchesCount(previousTouches) > 0)
  {
    if (previousTouches[0].m_id == m_touches[1].m_id)
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
  return static_cast<size_t>(GetFirstMaskedPointer() != INVALID_MASKED_POINTER) +
         static_cast<size_t>(GetSecondMaskedPointer() != INVALID_MASKED_POINTER);
}

void TouchEvent::Swap()
{
  auto swapIndex = [](uint8_t index) -> uint8_t
  {
    if (index == INVALID_MASKED_POINTER)
      return index;

    return index ^ 0x1;
  };

  std::swap(m_touches[0], m_touches[1]);
  SetFirstMaskedPointer(swapIndex(GetFirstMaskedPointer()));
  SetSecondMaskedPointer(swapIndex(GetSecondMaskedPointer()));
}

std::string DebugPrint(Touch const & t)
{
  return DebugPrint(t.m_location) + "; " + std::to_string(t.m_id) + "; " + std::to_string(t.m_force);
}

std::string DebugPrint(TouchEvent const & e)
{
  return std::to_string(e.m_type) + "; { " + DebugPrint(e.m_touches[0]) + " }";
}

UserEventStream::UserEventStream(AnimationSystem & animationSystem)
  : m_state(STATE_EMPTY)
  , m_animationSystem(animationSystem)
  , m_startDragOrg(m2::PointD::Zero())
  , m_startDoubleTapAndHold(m2::PointD::Zero())
{}

void UserEventStream::AddEvent(drape_ptr<UserEvent> && event)
{
  std::lock_guard guard(m_lock);
  m_events.emplace_back(std::move(event));
}

ScreenBase const & UserEventStream::ProcessEvents(bool & modelViewChanged, bool & viewportChanged, bool & activeFrame,
                                                  AnimationClock::time_point now)
{
  TEventsList events;
  {
    std::lock_guard guard(m_lock);
    std::swap(m_events, events);
  }

  m2::RectD const prevPixelRect = GetCurrentScreen().PixelRect();

  activeFrame = false;
  viewportChanged = false;
  modelViewChanged = !events.empty() || IsInUserAction();

  for (auto const & e : events)
  {
    m_eventTime = e->GetRequestTime();
    switch (e->GetType())
    {
    case UserEvent::EventType::Scale:
    {
      ref_ptr<ScaleEvent> scaleEvent = make_ref(e);
      OnSetScale(scaleEvent);
      TouchCancel(m_touches);
    }
    break;
    case UserEvent::EventType::Move:
    {
      ref_ptr<MoveEvent> moveEvent = make_ref(e);
      OnMove(moveEvent);
      TouchCancel(m_touches);
    }
    break;
    case UserEvent::EventType::Resize:
    {
      ref_ptr<ResizeEvent> resizeEvent = make_ref(e);
      m_navigator.OnSize(resizeEvent->GetWidth(), resizeEvent->GetHeight());
      if (!m_visibleViewport.IsValid())
        m_visibleViewport = m2::RectD(0, 0, resizeEvent->GetWidth(), resizeEvent->GetHeight());
      viewportChanged = true;
      TouchCancel(m_touches);
      if (m_state == STATE_DOUBLE_TAP_HOLD)
        EndDoubleTapAndHold(m_touches[0]);
    }
    break;
    case UserEvent::EventType::SetAnyRect:
    {
      m_needTrackCenter = false;
      ref_ptr<SetAnyRectEvent> anyRectEvent = make_ref(e);
      OnSetAnyRect(anyRectEvent);
      TouchCancel(m_touches);
    }
    break;
    case UserEvent::EventType::SetRect:
    {
      m_needTrackCenter = false;
      ref_ptr<SetRectEvent> rectEvent = make_ref(e);
      OnSetRect(rectEvent);
      TouchCancel(m_touches);
    }
    break;
    case UserEvent::EventType::SetCenter:
    {
      m_needTrackCenter = false;
      ref_ptr<SetCenterEvent> centerEvent = make_ref(e);
      OnSetCenter(centerEvent);
      TouchCancel(m_touches);
    }
    break;
    case UserEvent::EventType::Touch:
    {
      m_needTrackCenter = false;
      ref_ptr<TouchEvent> touchEvent = make_ref(e);
      ProcessTouch(*touchEvent);
    }
    break;
    case UserEvent::EventType::Rotate:
    {
      m_needTrackCenter = false;
      ref_ptr<RotateEvent> rotateEvent = make_ref(e);
      OnRotate(rotateEvent);
    }
    break;
    case UserEvent::EventType::FollowAndRotate:
    {
      m_needTrackCenter = false;
      ref_ptr<FollowAndRotateEvent> followEvent = make_ref(e);
      SetFollowAndRotate(followEvent->GetUserPos(), followEvent->GetPixelZero(), followEvent->GetAzimuth(),
                         followEvent->GetPreferredZoomLevel(), followEvent->GetAutoScale(), true /* isAnim */,
                         followEvent->GetArrowTarget());
    }
    break;
    case UserEvent::EventType::AutoPerspective:
    {
      m_needTrackCenter = false;
      ref_ptr<SetAutoPerspectiveEvent> perspectiveEvent = make_ref(e);
      SetAutoPerspective(perspectiveEvent->IsAutoPerspective());
    }
    break;
    case UserEvent::EventType::VisibleViewport:
    {
      ref_ptr<SetVisibleViewportEvent> viewportEvent = make_ref(e);
      OnNewVisibleViewport(viewportEvent);
    }
    break;
    case UserEvent::EventType::Scroll:
    {
      ref_ptr<ScrollEvent> scrollEvent = make_ref(e);
      OnScroll(scrollEvent);
      TouchCancel(m_touches);
    }
    break;
    case UserEvent::EventType::ActiveFrame:
    {
      activeFrame = true;
    }
    break;
    default: ASSERT(false, ()); break;
    }
  }

  m_animationSystem.Advance(now);
  auto const & current = GetCurrentScreen();
  if (auto screen = m_animationSystem.GetScreen(current); screen && *screen != current)
  {
    // Keep the unwrapped origin. Normalizing it would invalidate all visible tile X keys.
    m_navigator.SetFromScreen(*screen);
    modelViewChanged = true;
  }

  if (GetValidTouchesCount(m_touches) == 1)
  {
    if (m_state == STATE_WAIT_DOUBLE_TAP)
      DetectShortTap(m_touches[0]);
    else if (m_state == STATE_TAP_DETECTION)
      DetectLongTap(m_touches[0]);
  }

  if (!viewportChanged)
  {
    double constexpr kEps = 1e-5;
    viewportChanged = !m2::IsEqualSize(prevPixelRect, GetCurrentScreen().PixelRect(), kEps, kEps);
  }

  return m_navigator.Screen();
}

ScreenBase const & UserEventStream::GetCurrentScreen() const
{
  return m_navigator.Screen();
}

m2::RectD const & UserEventStream::GetVisibleViewport() const
{
  return m_visibleViewport;
}

void UserEventStream::OnSetScale(ref_ptr<ScaleEvent> scaleEvent)
{
  double const factor = scaleEvent->GetFactor();
  auto scaleCenter = scaleEvent->GetPxPoint();
  if (m_listener)
  {
    m_listener->CorrectScalePoint(scaleCenter);
    m_listener->OnScaleRequested();
  }

  if (!scaleEvent->IsAnim())
  {
    m_animationSystem.CancelMapAnimation();
    m_navigator.Scale(scaleCenter, factor);
    return;
  }

  auto const & current = GetCurrentScreen();
  auto anchor = current.PtoG(current.P3dtoP(scaleCenter));
  if (m_listener)
    m_listener->CorrectGlobalScalePoint(anchor);
  anchor = AdjustPointForViewport(anchor, current);

  ScreenBase target = current;
  if (m_animationSystem.HasMapAnimation(MapAnimation::Type::Zoom))
    target = GetTargetScreen();
  target.MatchGandP3d(anchor, scaleCenter);
  if (!ApplyScale(scaleCenter, factor, target))
    return;

  if (target == current)
  {
    m_animationSystem.CancelMapAnimation();
    return;
  }

  m_animationSystem.StartMapAnimation(
      {MapAnimation::Type::Zoom, current, target, m_eventTime, kStepAnimationTimeSec, scaleCenter});
}

void UserEventStream::OnMove(ref_ptr<MoveEvent> moveEvent)
{
  double const factorX = moveEvent->GetFactorX();
  double const factorY = moveEvent->GetFactorY();

  ScreenBase screen = GetCurrentScreen();
  if (m_animationSystem.HasMapAnimation(MapAnimation::Type::Pan))
    screen = GetTargetScreen();
  auto const & rect = screen.PixelRectIn3d();
  screen.Move(factorX * rect.SizeX(), -factorY * rect.SizeY());

  SetScreen(screen, moveEvent->IsAnim() ? kStepAnimationTimeSec : 0.0, std::nullopt, MapAnimation::Type::Pan);
}

void UserEventStream::OnSetAnyRect(ref_ptr<SetAnyRectEvent> anyRectEvent)
{
  SetRect(anyRectEvent->GetRect(), anyRectEvent->IsAnim(), anyRectEvent->FitInViewport(),
          anyRectEvent->UseVisibleViewport());
}

void UserEventStream::OnSetRect(ref_ptr<SetRectEvent> rectEvent)
{
  SetRect(rectEvent->GetRect(), rectEvent->GetZoom(), rectEvent->GetApplyRotation(), rectEvent->IsAnim(),
          rectEvent->UseVisibleViewport(), rectEvent->GetArrowTarget());
}

void UserEventStream::OnSetCenter(ref_ptr<SetCenterEvent> centerEvent)
{
  m2::PointD center = centerEvent->GetCenter();
  auto const zoom = centerEvent->GetZoom();
  auto const scaleFactor = centerEvent->GetScaleFactor();

  center.x = mercator::NearestWrapX(center.x, GetCurrentScreen().GetOrg().x);

  if (centerEvent->TrackVisibleViewport())
  {
    m_needTrackCenter = true;
    m_trackedCenter = center;
  }

  ScreenBase screen = GetCurrentScreen();

  if (zoom != kDoNotChangeZoom)
  {
    screen.SetFromParams(center, screen.GetAngle(), GetScreenScale(zoom));
    screen.MatchGandP3d(center, m_visibleViewport.Center());
  }
  else if (scaleFactor > 0.0)
  {
    screen.SetOrg(center);
    ApplyScale(m_visibleViewport.Center(), scaleFactor, screen);
  }
  else
  {
    screen = GetTargetScreen();
    screen.MatchGandP3d(center, m_visibleViewport.Center());
  }
  // Near the poles, centering the point can move the view beyond the world, which the navigator doesn't accept.
  ShrinkAndScaleInto(screen, GetWorldRect());

  double duration = centerEvent->IsAnim() ? kCameraAnimationTimeSec : 0.0;
  if (zoom == kDoNotChangeZoom && scaleFactor <= 0.0)
    duration = KeepZoomDuration(duration, screen.GetScale());
  SetScreen(screen, duration, centerEvent->GetArrowTarget());
}

void UserEventStream::OnRotate(ref_ptr<RotateEvent> rotateEvent)
{
  SetAngle(rotateEvent->GetTargetAzimuth(), rotateEvent->IsAnim(), rotateEvent->GetArrowTarget());
}

void UserEventStream::OnNewVisibleViewport(ref_ptr<SetVisibleViewportEvent> viewportEvent)
{
  auto const previousViewport = m_visibleViewport;
  m_visibleViewport = viewportEvent->GetRect();
  if (m_visibleViewport == previousViewport)
    return;

  m2::PointD offset;
  bool const hasOffset =
      m_listener && m_listener->OnNewVisibleViewport(previousViewport, m_visibleViewport, !m_needTrackCenter, offset);
  if (m_needTrackCenter ? previousViewport.Center() == m_visibleViewport.Center() : !hasOffset)
    return;

  ScreenBase target = GetTargetScreen();
  if (m_needTrackCenter)
    target.MatchGandP3d(m_trackedCenter, m_visibleViewport.Center());
  else
    target.MoveG(offset);

  // A layout change can retarget a camera move without extending its deadline.
  double const duration =
      m_animationSystem.IsCameraMoving() && !m_animationSystem.HasMapAnimation(MapAnimation::Type::Fling)
          ? std::min(kCameraAnimationTimeSec, m_animationSystem.GetRemainingCameraTime(m_eventTime))
          : 0.0;
  SetScreen(target, duration);
}

void UserEventStream::OnScroll(ref_ptr<ScrollEvent> scrollEvent)
{
  double const distanceX = scrollEvent->GetDistanceX();
  double const distanceY = scrollEvent->GetDistanceY();

  ScreenBase screen = GetCurrentScreen();
  screen.Move(-distanceX, -distanceY);

  if (m_listener)
    m_listener->OnScrolled({-distanceX, -distanceY});

  SetScreen(screen);
}

void UserEventStream::SetAngle(double azimuth, bool isAnim, std::optional<ArrowPose> const & arrowTarget)
{
  auto const & screen = GetCurrentScreen();
  auto const anchor = m_visibleViewport.Center();
  auto const point = screen.PtoG(screen.P3dtoP(anchor));
  // Follow-and-rotate takes a heading, which has the opposite sign to a screen angle.
  SetFollowAndRotate(point, anchor, -azimuth, kDoNotChangeZoom, std::nullopt, isAnim, arrowTarget);
}

void UserEventStream::SetRect(m2::RectD rect, int zoom, bool applyRotation, bool isAnim, bool useVisibleViewport,
                              std::optional<ArrowPose> const & arrowTarget)
{
  // For animated transitions, choose the shortest horizontal path across the antimeridian.
  if (isAnim)
  {
    double const offset = mercator::NearestWrapX(rect.Center().x, GetCurrentScreen().GetOrg().x) - rect.Center().x;
    if (offset != 0.0)
      rect.Offset(offset, 0.0);
  }

  CheckMinGlobalRect(rect, kDefault3dScale);
  CheckMinMaxVisibleScale(rect, zoom, kDefault3dScale);
  m2::AnyRectD targetRect = applyRotation ? ToRotated(m_navigator, rect) : m2::AnyRectD(rect);
  SetRect(targetRect, isAnim, true /* fitInViewport */, useVisibleViewport, arrowTarget);
}

void UserEventStream::SetRect(m2::AnyRectD const & rect, bool isAnim, bool fitInViewport, bool useVisibleViewport,
                              std::optional<ArrowPose> const & arrowTarget)
{
  ScreenBase tmp = GetCurrentScreen();
  if (fitInViewport)
  {
    auto viewportRect = tmp.PixelRectIn3d();
    if (useVisibleViewport && m_visibleViewport.IsValid())
      viewportRect = m_visibleViewport;
    tmp.SetFromRects(rect, viewportRect);
    tmp.MatchGandP3d(rect.GlobalCenter(), viewportRect.Center());
  }
  else
  {
    tmp.SetFromRects(rect, tmp.PixelRect());
  }
  SetScreen(tmp, isAnim ? kCameraAnimationTimeSec : 0.0, arrowTarget);
}

void UserEventStream::SetScreen(ScreenBase const & target, double duration,
                                std::optional<ArrowPose> const & arrowTarget, MapAnimation::Type type,
                                std::optional<m2::PointD> anchor)
{
  auto const & current = GetCurrentScreen();
  bool const animate = duration > 0.0 && current != target && IsLocalCameraMove(current, target);
  if (arrowTarget)
    m_animationSystem.AnimateArrowTo(*arrowTarget, m_eventTime, animate ? duration : 0.0);
  if (animate)
  {
    m_animationSystem.StartMapAnimation({type, current, target, m_eventTime, duration, anchor});
    return;
  }

  m_animationSystem.CancelMapAnimation();
  m_navigator.SetFromScreen(target);
}

void UserEventStream::SetFollowAndRotate(m2::PointD const & userPos, m2::PointD const & pixelPos, double azimuth,
                                         int preferredZoomLevel, std::optional<double> autoScale, bool isAnim,
                                         std::optional<ArrowPose> const & arrowTarget)
{
  auto const & current = GetCurrentScreen();
  auto const position = AdjustPointForViewport(userPos, current);
  ScreenBase target = current;
  bool const keepZoom = !autoScale && preferredZoomLevel == kDoNotChangeZoom;
  double const scale = autoScale ? *autoScale
                     : keepZoom  ? GetTargetScreen().GetScale()
                                 : GetScreenScale(preferredZoomLevel);
  target.SetFromParams(position, -azimuth, scale);
  target.MatchGandP3d(position, pixelPos);

  double duration = autoScale ? kAutoZoomAnimationTimeSec : kCameraAnimationTimeSec;
  if (keepZoom)
    duration = KeepZoomDuration(duration, scale);

  SetScreen(target, isAnim ? duration : 0.0, arrowTarget, MapAnimation::Type::Camera, pixelPos);
}

void UserEventStream::SetAutoPerspective(bool isAutoPerspective)
{
  if (isAutoPerspective)
    m_navigator.Enable3dMode();
  else
    m_navigator.Disable3dMode();
  m_navigator.SetAutoPerspective(isAutoPerspective);
}

double UserEventStream::KeepZoomDuration(double duration, double targetScale) const
{
  // A recenter that inherits a zoom target finishes on that zoom's original deadline.
  if (targetScale == GetCurrentScreen().GetScale())
    return duration;
  return std::min(duration, m_animationSystem.GetRemainingCameraTime(m_eventTime));
}

ScreenBase UserEventStream::GetTargetScreen() const
{
  return m_animationSystem.GetTargetScreen(GetCurrentScreen());
}

void UserEventStream::ProcessTouch(TouchEvent const & touch)
{
  ASSERT(touch.GetFirstTouch().m_id != -1, ());

  TouchEvent touchEvent = touch;
  touchEvent.PrepareTouches(m_touches);
  bool isMapTouch = false;

  switch (touchEvent.GetTouchType())
  {
  case TouchEvent::TOUCH_DOWN: isMapTouch = TouchDown(touchEvent.GetTouches()); break;
  case TouchEvent::TOUCH_MOVE: isMapTouch = TouchMove(touchEvent.GetTouches()); break;
  case TouchEvent::TOUCH_CANCEL: isMapTouch = TouchCancel(touchEvent.GetTouches()); break;
  case TouchEvent::TOUCH_UP: isMapTouch = TouchUp(touchEvent.GetTouches()); break;
  default: ASSERT(false, ()); break;
  }

  if (m_listener)
    m_listener->OnTouchMapAction(touchEvent.GetTouchType(), isMapTouch);
}

bool UserEventStream::TouchDown(std::array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;

  if (m_animationSystem.HasMapAnimation(MapAnimation::Type::Fling))
    m_animationSystem.CancelMapAnimation();

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
        {
          isMapTouch = false;
        }
      }
    }
  }
  else if (touchCount == 2)
  {
    switch (m_state)
    {
    case STATE_EMPTY: BeginTwoFingersTap(touches[0], touches[1]); break;
    case STATE_FILTER:
      CancelFilter(touches[0]);
      BeginScale(touches[0], touches[1]);
      break;
    case STATE_TAP_DETECTION:
    case STATE_WAIT_DOUBLE_TAP:
    case STATE_WAIT_DOUBLE_TAP_HOLD:
      CancelTapDetector();
      BeginTwoFingersTap(touches[0], touches[1]);
      break;
    case STATE_DOUBLE_TAP_HOLD:
      EndDoubleTapAndHold(touches[0]);
      BeginScale(touches[0], touches[1]);
      break;
    case STATE_DRAG:
      isMapTouch = EndDrag(touches[0], true /* cancelled */);
      BeginScale(touches[0], touches[1]);
      break;
    default: break;
    }
  }

  UpdateTouches(touches);
  return isMapTouch;
}

bool UserEventStream::CheckDrag(std::array<Touch, 2> const & touches, double threshold) const
{
  return m_startDragOrg.SquaredLength(m2::PointD(touches[0].m_location)) > threshold;
}

bool UserEventStream::TouchMove(std::array<Touch, 2> const & touches)
{
  double const dragThreshold = math::Pow2(VisualParams::Instance().GetDragThreshold());
  size_t const touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;

  switch (m_state)
  {
  case STATE_EMPTY:
    if (touchCount == 1)
      if (CheckDrag(touches, dragThreshold))
        BeginDrag(touches[0]);
      else
        isMapTouch = false;
    else
      BeginScale(touches[0], touches[1]);
    break;
  case STATE_TAP_TWO_FINGERS:
    if (touchCount == 2)
    {
      float const threshold = static_cast<float>(dragThreshold);
      if (m_twoFingersTouches[0].SquaredLength(touches[0].m_location) > threshold ||
          m_twoFingersTouches[1].SquaredLength(touches[1].m_location) > threshold)
      {
        BeginScale(touches[0], touches[1]);
      }
      else
        isMapTouch = false;
    }
    break;
  case STATE_FILTER:
    ASSERT_EQUAL(touchCount, 1, ());
    isMapTouch = false;
    break;
  case STATE_TAP_DETECTION:
  case STATE_WAIT_DOUBLE_TAP:
    if (CheckDrag(touches, dragThreshold))
      CancelTapDetector();
    else
      isMapTouch = false;
    break;
  case STATE_WAIT_DOUBLE_TAP_HOLD:
    if (CheckDrag(touches, dragThreshold))
      StartDoubleTapAndHold(touches[0]);
    break;
  case STATE_DOUBLE_TAP_HOLD: UpdateDoubleTapAndHold(touches[0]); break;
  case STATE_DRAG:
    if (touchCount > 1)
    {
      ASSERT_EQUAL(GetValidTouchesCount(m_touches), 1, ());
      EndDrag(m_touches[0], true /* cancelled */);
    }
    else
    {
      Drag(touches[0]);
    }
    break;
  case STATE_SCALE:
    if (touchCount < 2)
    {
      ASSERT_EQUAL(GetValidTouchesCount(m_touches), 2, ());
      EndScale(m_touches[0], m_touches[1]);
    }
    else
    {
      Scale(touches[0], touches[1]);
    }
    break;
  default: ASSERT(false, ()); break;
  }

  UpdateTouches(touches);
  return isMapTouch;
}

bool UserEventStream::TouchCancel(std::array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);
  UNUSED_VALUE(touchCount);
  bool isMapTouch = true;
  switch (m_state)
  {
  case STATE_EMPTY:
  case STATE_WAIT_DOUBLE_TAP:
  case STATE_TAP_TWO_FINGERS: isMapTouch = false; break;
  case STATE_WAIT_DOUBLE_TAP_HOLD:
  case STATE_DOUBLE_TAP_HOLD:
    // Do nothing here.
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
    isMapTouch = EndDrag(touches[0], true /* cancelled */);
    break;
  case STATE_SCALE:
    ASSERT_EQUAL(touchCount, 2, ());
    EndScale(touches[0], touches[1]);
    break;
  default: ASSERT(false, ()); break;
  }
  UpdateTouches(touches);
  return isMapTouch;
}

bool UserEventStream::TouchUp(std::array<Touch, 2> const & touches)
{
  size_t touchCount = GetValidTouchesCount(touches);
  bool isMapTouch = true;
  switch (m_state)
  {
  case STATE_EMPTY:
  case STATE_WAIT_DOUBLE_TAP:
    isMapTouch = false;
    // Can be if long tap or double tap detected.
    break;
  case STATE_FILTER:
    ASSERT_EQUAL(touchCount, 1, ());
    EndFilter(touches[0]);
    isMapTouch = false;
    break;
  case STATE_TAP_DETECTION:
    if (touchCount == 1)
      EndTapDetector(touches[0]);
    else
      CancelTapDetector();
    break;
  case STATE_TAP_TWO_FINGERS:
    if (touchCount == 2)
      EndTwoFingersTap();
    break;
  case STATE_WAIT_DOUBLE_TAP_HOLD:
    ASSERT_EQUAL(touchCount, 1, ());
    PerformDoubleTap(touches[0]);
    break;
  case STATE_DOUBLE_TAP_HOLD: EndDoubleTapAndHold(touches[0]); break;
  case STATE_DRAG:
    ASSERT_EQUAL(touchCount, 1, ());
    isMapTouch = EndDrag(touches[0], false /* cancelled */);
    break;
  case STATE_SCALE:
    if (touchCount < 2)
    {
      ASSERT_EQUAL(GetValidTouchesCount(m_touches), 2, ());
      EndScale(m_touches[0], m_touches[1]);
    }
    else
    {
      EndScale(touches[0], touches[1]);
    }
    break;
  default: ASSERT(false, ()); break;
  }

  UpdateTouches(touches);
  return isMapTouch;
}

void UserEventStream::UpdateTouches(std::array<Touch, 2> const & touches)
{
  m_touches = touches;
}

void UserEventStream::BeginTwoFingersTap(Touch const & t1, Touch const & t2)
{
  TEST_CALL(TWO_FINGERS_TAP);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  m_state = STATE_TAP_TWO_FINGERS;
  m_twoFingersTouches[0] = t1.m_location;
  m_twoFingersTouches[1] = t2.m_location;
}

void UserEventStream::EndTwoFingersTap()
{
  ASSERT_EQUAL(m_state, STATE_TAP_TWO_FINGERS, ());
  m_state = STATE_EMPTY;

  if (m_listener)
    m_listener->OnTwoFingersTap();
}

void UserEventStream::BeginDrag(Touch const & t)
{
  TEST_CALL(BEGIN_DRAG);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  m_state = STATE_DRAG;
  m_startDragOrg = m_navigator.Screen().GetOrg();
  if (m_listener)
    m_listener->OnDragStarted();
  m_navigator.StartDrag(m2::PointD(t.m_location));

  m_animationSystem.CancelMapAnimation();
  if (m_kineticScrollEnabled && !m_scroller.IsActive())
    m_scroller.Init(m_navigator.Screen(), m_eventTime);
}

void UserEventStream::Drag(Touch const & t)
{
  TEST_CALL(DRAG);
  ASSERT_EQUAL(m_state, STATE_DRAG, ());
  m_navigator.DoDrag(m2::PointD(t.m_location));

  if (m_kineticScrollEnabled && m_scroller.IsActive())
    m_scroller.Update(m_navigator.Screen(), m_eventTime);
}

bool UserEventStream::EndDrag(Touch const & t, bool cancelled)
{
  TEST_CALL(END_DRAG);
  ASSERT_EQUAL(m_state, STATE_DRAG, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnDragEnded(m_navigator.GtoP(m_navigator.Screen().GetOrg()) - m_navigator.GtoP(m_startDragOrg));

  m_startDragOrg = m2::PointD::Zero();
  m_navigator.StopDrag(m2::PointD(t.m_location));

  if (!cancelled && m_kineticScrollEnabled && m_scroller.IsActive() &&
      m_kineticTimer.ElapsedMilliseconds() >= kKineticDelayMs)
  {
    if (auto animation = m_scroller.CreateKineticAnimation(m_navigator.Screen(), m_eventTime))
    {
      m_animationSystem.StartMapAnimation(std::move(*animation));
      return false;
    }
  }

  m_scroller.Cancel();
  return true;
}

void UserEventStream::BeginScale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(BEGIN_SCALE);

  if (m_state == STATE_SCALE)
  {
    Scale(t1, t2);
    return;
  }

  ASSERT(m_state == STATE_EMPTY || m_state == STATE_TAP_TWO_FINGERS, ());
  m_state = STATE_SCALE;
  m2::PointD touch1(t1.m_location);
  m2::PointD touch2(t2.m_location);

  if (m_listener)
  {
    m_listener->OnScaleStarted();
    m_listener->CorrectScalePoint(touch1, touch2);
  }

  m_animationSystem.CancelMapAnimation();
  m_navigator.StartScale(touch1, touch2);
}

void UserEventStream::Scale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(SCALE);
  ASSERT_EQUAL(m_state, STATE_SCALE, ());

  m2::PointD touch1(t1.m_location);
  m2::PointD touch2(t2.m_location);

  if (m_listener)
  {
    if (m_navigator.IsRotatingDuringScale())
      m_listener->OnRotated();

    m_listener->CorrectScalePoint(touch1, touch2);
  }

  m_navigator.DoScale(touch1, touch2);
}

void UserEventStream::EndScale(Touch const & t1, Touch const & t2)
{
  TEST_CALL(END_SCALE);
  ASSERT_EQUAL(m_state, STATE_SCALE, ());
  m_state = STATE_EMPTY;

  m2::PointD touch1(t1.m_location);
  m2::PointD touch2(t2.m_location);

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
  if (DetectForceTap(touch))
    return;

  if (m_touchTimer.ElapsedMilliseconds() > kDoubleTapPauseMs)
  {
    m_state = STATE_EMPTY;
    if (m_listener)
      m_listener->OnTap(m2::PointD(touch.m_location), false /* isLongTap */);
  }
}

void UserEventStream::DetectLongTap(Touch const & touch)
{
  ASSERT_EQUAL(m_state, STATE_TAP_DETECTION, ());

  if (DetectForceTap(touch))
    return;

  if (m_touchTimer.ElapsedMilliseconds() > kLongTouchMs)
  {
    TEST_CALL(LONG_TAP_DETECTED);
    m_state = STATE_EMPTY;
    if (m_listener)
      m_listener->OnTap(m2::PointD(touch.m_location), true /* isLongTap */);
  }
}

bool UserEventStream::DetectDoubleTap(Touch const & touch)
{
  if (m_state != STATE_WAIT_DOUBLE_TAP || m_touchTimer.ElapsedMilliseconds() > kDoubleTapPauseMs)
    return false;

  m_state = STATE_WAIT_DOUBLE_TAP_HOLD;
  return true;
}

void UserEventStream::PerformDoubleTap(Touch const & touch)
{
  ASSERT_EQUAL(m_state, STATE_WAIT_DOUBLE_TAP_HOLD, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnDoubleTap(m2::PointD(touch.m_location));
}

bool UserEventStream::DetectForceTap(Touch const & touch)
{
  if (touch.m_force >= kForceTapThreshold)
  {
    m_state = STATE_EMPTY;
    if (m_listener)
      m_listener->OnForceTap(m2::PointD(touch.m_location));
    return true;
  }

  return false;
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
  ASSERT(m_state == STATE_TAP_DETECTION || m_state == STATE_WAIT_DOUBLE_TAP || m_state == STATE_WAIT_DOUBLE_TAP_HOLD,
         ());
  m_state = STATE_EMPTY;
}

bool UserEventStream::TryBeginFilter(Touch const & t)
{
  TEST_CALL(TRY_FILTER);
  ASSERT_EQUAL(m_state, STATE_EMPTY, ());
  if (m_listener && m_listener->OnSingleTouchFiltrate(m2::PointD(t.m_location), TouchEvent::TOUCH_DOWN))
  {
    m_state = STATE_FILTER;
    return true;
  }

  return false;
}

void UserEventStream::EndFilter(Touch const & t)
{
  TEST_CALL(END_FILTER);
  ASSERT_EQUAL(m_state, STATE_FILTER, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnSingleTouchFiltrate(m2::PointD(t.m_location), TouchEvent::TOUCH_UP);
}

void UserEventStream::CancelFilter(Touch const & t)
{
  TEST_CALL(CANCEL_FILTER);
  ASSERT_EQUAL(m_state, STATE_FILTER, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnSingleTouchFiltrate(m2::PointD(t.m_location), TouchEvent::TOUCH_CANCEL);
}

void UserEventStream::StartDoubleTapAndHold(Touch const & touch)
{
  TEST_CALL(BEGIN_DOUBLE_TAP_AND_HOLD);
  ASSERT_EQUAL(m_state, STATE_WAIT_DOUBLE_TAP_HOLD, ());
  m_state = STATE_DOUBLE_TAP_HOLD;
  m_animationSystem.CancelMapAnimation();
  m_startDoubleTapAndHold = m_startDragOrg;
  if (m_listener)
    m_listener->OnScaleStarted();
}

void UserEventStream::UpdateDoubleTapAndHold(Touch const & touch)
{
  TEST_CALL(DOUBLE_TAP_AND_HOLD);
  ASSERT_EQUAL(m_state, STATE_DOUBLE_TAP_HOLD, ());
  float const kPowerModifier = 10.0f;
  double const scaleFactor = exp(kPowerModifier * (touch.m_location.y - m_startDoubleTapAndHold.y) /
                                 GetCurrentScreen().PixelRectIn3d().SizeY());
  m_startDoubleTapAndHold = touch.m_location;

  m2::PointD scaleCenter = m_startDragOrg;
  if (m_listener)
    m_listener->CorrectScalePoint(scaleCenter);
  m_navigator.Scale(scaleCenter, scaleFactor);
}

void UserEventStream::EndDoubleTapAndHold(Touch const & touch)
{
  TEST_CALL(END_DOUBLE_TAP_AND_HOLD);
  ASSERT_EQUAL(m_state, STATE_DOUBLE_TAP_HOLD, ());
  m_state = STATE_EMPTY;
  if (m_listener)
    m_listener->OnScaleEnded();
}

bool UserEventStream::IsInUserAction() const
{
  return m_state == STATE_DRAG || m_state == STATE_SCALE || m_state == STATE_DOUBLE_TAP_HOLD;
}

bool UserEventStream::IsWaitingForActionCompletion() const
{
  return m_state != STATE_EMPTY;
}

void UserEventStream::SetKineticScrollEnabled(bool enabled)
{
  m_kineticScrollEnabled = enabled;
  m_kineticTimer.Reset();
  if (!m_kineticScrollEnabled)
  {
    m_scroller.Cancel();
    if (m_animationSystem.HasMapAnimation(MapAnimation::Type::Fling))
      m_animationSystem.CancelMapAnimation();
  }
}
}  // namespace df
