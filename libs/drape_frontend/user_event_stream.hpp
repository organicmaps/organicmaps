#pragma once

#include "drape_frontend/animation/arrow_animation.hpp"
#include "drape_frontend/kinetic_scroller.hpp"
#include "drape_frontend/navigator.hpp"

#include "drape/pointers.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/timer.hpp"

#include <array>
#include <bitset>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace df
{
class AnimationSystem;
int const kDoNotChangeZoom = -1;

class UserEvent
{
public:
  enum class EventType
  {
    Touch,
    Scale,
    SetCenter,
    SetRect,
    SetAnyRect,
    Resize,
    Rotate,
    FollowAndRotate,
    AutoPerspective,
    VisibleViewport,
    Move,
    Scroll,
    ActiveFrame
  };

  virtual ~UserEvent() = default;
  virtual EventType GetType() const = 0;
  AnimationClock::time_point GetRequestTime() const { return m_requestedAt; }

private:
  AnimationClock::time_point const m_requestedAt = AnimationClock::now();
};

struct Touch
{
  m2::PointF m_location = m2::PointF::Zero();
  int64_t m_id = -1;    // if id == -1 then touch is invalid
  float m_force = 0.0;  // relative force of touch [0.0 - 1.0]

  friend std::string DebugPrint(Touch const & t);
};

class TouchEvent : public UserEvent
{
public:
  TouchEvent() : m_type(TOUCH_CANCEL), m_timeStamp(base::Timer::LocalTime()), m_pointersMask(0xFFFF) {}

  enum ETouchType
  {
    TOUCH_NONE,
    TOUCH_DOWN,
    TOUCH_MOVE,
    TOUCH_UP,
    TOUCH_CANCEL,
  };

  static uint8_t constexpr INVALID_MASKED_POINTER = 0xFF;

  EventType GetType() const override { return UserEvent::EventType::Touch; }

  ETouchType GetTouchType() const { return m_type; }
  void SetTouchType(ETouchType touchType) { m_type = touchType; }

  double GetTimeStamp() const { return m_timeStamp; }
  void SetTimeStamp(double timeStamp) { m_timeStamp = timeStamp; }

  std::array<Touch, 2> const & GetTouches() const { return m_touches; }

  Touch const & GetFirstTouch() const { return m_touches[0]; }
  Touch const & GetSecondTouch() const { return m_touches[1]; }

  void SetFirstTouch(Touch const & touch);
  void SetSecondTouch(Touch const & touch);

  void PrepareTouches(std::array<Touch, 2> const & previousToches);

  /// Methods for work with current touches
  /// For example : user put down one finger. We will have one touch in m_touches
  /// and GetFirstMaskedPointer return index of this pointer in m_touches (0 in this case)
  /// Then user puts down the second finger. m_touches will have 2 valid elements, but new finger only one.
  /// In this case GetFirstMaskedPointer returns index of new pointer.
  /// If user put down both fingers simultaneously, then GetFirst and GetSecond
  /// will return valid not equal INVALID_MASKED_POINTER
  void SetFirstMaskedPointer(uint8_t firstMask);
  uint8_t GetFirstMaskedPointer() const;
  void SetSecondMaskedPointer(uint8_t secondMask);
  uint8_t GetSecondMaskedPointer() const;
  size_t GetMaskedCount();

  friend std::string DebugPrint(TouchEvent const & e);

private:
  void Swap();

  ETouchType m_type;
  std::array<Touch, 2> m_touches;  // array of all touches
  double m_timeStamp;              // seconds
  uint16_t m_pointersMask;
};

class ScaleEvent : public UserEvent
{
public:
  ScaleEvent(double factor, m2::PointD const & pxPoint, bool isAnim)
    : m_factor(factor)
    , m_pxPoint(pxPoint)
    , m_isAnim(isAnim)
  {}

  EventType GetType() const override { return UserEvent::EventType::Scale; }

  double GetFactor() const { return m_factor; }
  m2::PointD const & GetPxPoint() const { return m_pxPoint; }
  bool IsAnim() const { return m_isAnim; }

private:
  double m_factor;
  m2::PointD m_pxPoint;
  bool m_isAnim;
};

class MoveEvent : public UserEvent
{
public:
  MoveEvent(double factorX, double factorY, bool isAnim) : m_factorX(factorX), m_factorY(factorY), m_isAnim(isAnim) {}

  EventType GetType() const override { return UserEvent::EventType::Move; }

  double GetFactorX() const { return m_factorX; }
  double GetFactorY() const { return m_factorY; }
  bool IsAnim() const { return m_isAnim; }

private:
  double m_factorX;
  double m_factorY;
  bool m_isAnim;
};

class SetCenterEvent : public UserEvent
{
public:
  SetCenterEvent(m2::PointD const & center, int zoom, bool isAnim, bool trackVisibleViewport,
                 std::optional<ArrowPose> const & arrowTarget = std::nullopt)
    : m_center(center)
    , m_zoom(zoom)
    , m_isAnim(isAnim)
    , m_trackVisibleViewport(trackVisibleViewport)
    , m_arrowTarget(arrowTarget)
  {}

  EventType GetType() const override { return UserEvent::EventType::SetCenter; }

  m2::PointD const & GetCenter() const { return m_center; }
  int GetZoom() const { return m_zoom; }
  bool IsAnim() const { return m_isAnim; }
  bool TrackVisibleViewport() const { return m_trackVisibleViewport; }
  std::optional<ArrowPose> const & GetArrowTarget() const { return m_arrowTarget; }

private:
  m2::PointD m_center;  // center point in mercator
  int m_zoom;           // if zoom == -1, then zoom level will not change
  bool m_isAnim;
  bool m_trackVisibleViewport;
  std::optional<ArrowPose> m_arrowTarget;
};

class SetRectEvent : public UserEvent
{
public:
  SetRectEvent(m2::RectD const & rect, bool rotate, int zoom, bool isAnim, bool useVisibleViewport,
               std::optional<ArrowPose> const & arrowTarget = std::nullopt)
    : m_rect(rect)
    , m_applyRotation(rotate)
    , m_zoom(zoom)
    , m_isAnim(isAnim)
    , m_useVisibleViewport(useVisibleViewport)
    , m_arrowTarget(arrowTarget)
  {}

  EventType GetType() const override { return UserEvent::EventType::SetRect; }

  m2::RectD const & GetRect() const { return m_rect; }
  bool GetApplyRotation() const { return m_applyRotation; }
  int GetZoom() const { return m_zoom; }
  bool IsAnim() const { return m_isAnim; }
  bool UseVisibleViewport() const { return m_useVisibleViewport; }
  std::optional<ArrowPose> const & GetArrowTarget() const { return m_arrowTarget; }

private:
  m2::RectD m_rect;      // destination mercator rect
  bool m_applyRotation;  // if true, current rotation will be apply to m_rect
  int m_zoom;            // if zoom == -1, then zoom level will'n change
  bool m_isAnim;
  bool m_useVisibleViewport;
  std::optional<ArrowPose> m_arrowTarget;
};

class SetAnyRectEvent : public UserEvent
{
public:
  SetAnyRectEvent(m2::AnyRectD const & rect, bool isAnim, bool fitInViewport, bool useVisibleViewport)
    : m_rect(rect)
    , m_isAnim(isAnim)
    , m_fitInViewport(fitInViewport)
    , m_useVisibleViewport(useVisibleViewport)
  {}

  EventType GetType() const override { return UserEvent::EventType::SetAnyRect; }

  m2::AnyRectD const & GetRect() const { return m_rect; }
  bool IsAnim() const { return m_isAnim; }
  bool FitInViewport() const { return m_fitInViewport; }
  bool UseVisibleViewport() const { return m_useVisibleViewport; }

private:
  m2::AnyRectD m_rect;  // destination mercator rect
  bool m_isAnim;
  bool m_fitInViewport;
  bool m_useVisibleViewport;
};

class FollowAndRotateEvent : public UserEvent
{
public:
  FollowAndRotateEvent(m2::PointD const & userPos, m2::PointD const & pixelZero, double azimuth, double autoScale,
                       std::optional<ArrowPose> const & arrowTarget = std::nullopt)
    : m_userPos(userPos)
    , m_pixelZero(pixelZero)
    , m_azimuth(azimuth)
    , m_preferredZoomLevel(kDoNotChangeZoom)
    , m_autoScale(autoScale)
    , m_arrowTarget(arrowTarget)
  {}

  FollowAndRotateEvent(m2::PointD const & userPos, m2::PointD const & pixelZero, double azimuth, int preferredZoomLevel,
                       std::optional<ArrowPose> const & arrowTarget = std::nullopt)
    : m_userPos(userPos)
    , m_pixelZero(pixelZero)
    , m_azimuth(azimuth)
    , m_preferredZoomLevel(preferredZoomLevel)
    , m_arrowTarget(arrowTarget)
  {}

  EventType GetType() const override { return UserEvent::EventType::FollowAndRotate; }

  m2::PointD const & GetUserPos() const { return m_userPos; }
  m2::PointD const & GetPixelZero() const { return m_pixelZero; }
  double GetAzimuth() const { return m_azimuth; }
  int GetPreferredZoomLevel() const { return m_preferredZoomLevel; }
  std::optional<double> GetAutoScale() const { return m_autoScale; }
  std::optional<ArrowPose> const & GetArrowTarget() const { return m_arrowTarget; }

private:
  m2::PointD m_userPos;
  m2::PointD m_pixelZero;
  double m_azimuth;
  int m_preferredZoomLevel;
  std::optional<double> m_autoScale;
  std::optional<ArrowPose> m_arrowTarget;
};

class SetAutoPerspectiveEvent : public UserEvent
{
public:
  explicit SetAutoPerspectiveEvent(bool isAutoPerspective) : m_isAutoPerspective(isAutoPerspective) {}

  EventType GetType() const override { return UserEvent::EventType::AutoPerspective; }

  bool IsAutoPerspective() const { return m_isAutoPerspective; }

private:
  bool m_isAutoPerspective;
};

class RotateEvent : public UserEvent
{
public:
  RotateEvent(double targetAzimuth, bool isAnim, std::optional<ArrowPose> const & arrowTarget = std::nullopt)
    : m_targetAzimuth(targetAzimuth)
    , m_isAnim(isAnim)
    , m_arrowTarget(arrowTarget)
  {}

  EventType GetType() const override { return UserEvent::EventType::Rotate; }

  bool IsAnim() const { return m_isAnim; }
  double GetTargetAzimuth() const { return m_targetAzimuth; }
  std::optional<ArrowPose> const & GetArrowTarget() const { return m_arrowTarget; }

private:
  double m_targetAzimuth;
  bool m_isAnim;
  std::optional<ArrowPose> m_arrowTarget;
};

class ResizeEvent : public UserEvent
{
public:
  ResizeEvent(uint32_t w, uint32_t h) : m_width(w), m_height(h) {}

  EventType GetType() const override { return UserEvent::EventType::Resize; }

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }

private:
  uint32_t m_width;
  uint32_t m_height;
};

class SetVisibleViewportEvent : public UserEvent
{
public:
  explicit SetVisibleViewportEvent(m2::RectD const & rect) : m_rect(rect) {}

  EventType GetType() const override { return UserEvent::EventType::VisibleViewport; }

  m2::RectD const & GetRect() const { return m_rect; }

private:
  m2::RectD m_rect;
};

class ScrollEvent : public UserEvent
{
public:
  ScrollEvent(double distanceX, double distanceY) : m_distanceX(distanceX), m_distanceY(distanceY) {}

  EventType GetType() const override { return UserEvent::EventType::Scroll; }

  double GetDistanceX() const { return m_distanceX; }
  double GetDistanceY() const { return m_distanceY; }

private:
  double m_distanceX;
  double m_distanceY;
};

// Doesn't have any payload, allows to unfreeze rendering in frontend_renderer
class ActiveFrameEvent : public UserEvent
{
public:
  explicit ActiveFrameEvent() {}

  EventType GetType() const override { return UserEvent::EventType::ActiveFrame; }
};

class UserEventStream
{
public:
  class Listener
  {
  public:
    virtual ~Listener() = default;

    virtual void OnTap(m2::PointD const & pt, bool isLong) = 0;
    virtual void OnForceTap(m2::PointD const & pt) = 0;
    virtual void OnDoubleTap(m2::PointD const & pt) = 0;
    virtual void OnTwoFingersTap() = 0;
    virtual bool OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type) = 0;
    virtual void OnDragStarted() = 0;
    virtual void OnDragEnded(m2::PointD const & distance) = 0;

    virtual void OnScaleStarted() = 0;
    virtual void OnRotated() = 0;
    virtual void OnScrolled(m2::PointD const & distance) = 0;
    virtual void CorrectScalePoint(m2::PointD & pt) const = 0;
    virtual void CorrectGlobalScalePoint(m2::PointD & pt) const = 0;
    virtual void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const = 0;
    virtual void OnScaleEnded() = 0;
    virtual void OnScaleRequested() = 0;

    virtual void OnTouchMapAction(TouchEvent::ETouchType touchType, bool isMapTouch) = 0;

    // Return a nonzero correction in the camera target's coordinates when a selection needs to move.
    virtual bool OnNewVisibleViewport(m2::RectD const & oldViewport, m2::RectD const & newViewport, bool needOffset,
                                      m2::PointD & gOffset) = 0;
  };

  explicit UserEventStream(AnimationSystem & animationSystem);

  void AddEvent(drape_ptr<UserEvent> && event);
  ScreenBase const & ProcessEvents(bool & modelViewChanged, bool & viewportChanged, bool & activeFrame,
                                   AnimationClock::time_point now);
  ScreenBase const & GetCurrentScreen() const;
  m2::RectD const & GetVisibleViewport() const;

  ScreenBase GetTargetScreen() const;

  bool IsInUserAction() const;
  bool IsWaitingForActionCompletion() const;

  void SetListener(ref_ptr<Listener> listener) { m_listener = listener; }

  void SetKineticScrollEnabled(bool enabled);

#ifdef DEBUG
  static char const * BEGIN_DRAG;
  static char const * DRAG;
  static char const * END_DRAG;
  static char const * BEGIN_SCALE;
  static char const * SCALE;
  static char const * END_SCALE;
  static char const * BEGIN_TAP_DETECTOR;
  static char const * LONG_TAP_DETECTED;
  static char const * SHORT_TAP_DETECTED;
  static char const * CANCEL_TAP_DETECTOR;
  static char const * TRY_FILTER;
  static char const * END_FILTER;
  static char const * CANCEL_FILTER;
  static char const * TWO_FINGERS_TAP;
  static char const * BEGIN_DOUBLE_TAP_AND_HOLD;
  static char const * DOUBLE_TAP_AND_HOLD;
  static char const * END_DOUBLE_TAP_AND_HOLD;

  using TTestBridge = std::function<void(char const * action)>;
  void SetTestBridge(TTestBridge const & fn) { m_testFn = fn; }
#endif

private:
  void OnSetScale(ref_ptr<ScaleEvent> scaleEvent);
  void OnMove(ref_ptr<MoveEvent> moveEvent);
  void OnSetAnyRect(ref_ptr<SetAnyRectEvent> anyRectEvent);
  void OnSetRect(ref_ptr<SetRectEvent> rectEvent);
  void OnSetCenter(ref_ptr<SetCenterEvent> centerEvent);
  void OnRotate(ref_ptr<RotateEvent> rotateEvent);
  void OnNewVisibleViewport(ref_ptr<SetVisibleViewportEvent> viewportEvent);
  void OnScroll(ref_ptr<ScrollEvent> scrollEvent);

  void SetAngle(double azimuth, bool isAnim, std::optional<ArrowPose> const & arrowTarget = std::nullopt);
  void SetRect(m2::RectD rect, int zoom, bool applyRotation, bool isAnim, bool useVisibleViewport,
               std::optional<ArrowPose> const & arrowTarget = std::nullopt);
  void SetRect(m2::AnyRectD const & rect, bool isAnim, bool fitInViewport, bool useVisibleViewport,
               std::optional<ArrowPose> const & arrowTarget = std::nullopt);

  void SetScreen(ScreenBase const & target, bool isAnim = false,
                 std::optional<ArrowPose> const & arrowTarget = std::nullopt,
                 MapAnimation::Type type = MapAnimation::Type::Camera, double maxScaleTime = kMaxAnimationTimeSec);
  void SetScreen(ScreenBase const & target, std::optional<MapAnimation> && animation,
                 std::optional<ArrowPose> const & arrowTarget);
  bool RetargetMove(ScreenBase const & target, std::optional<ArrowPose> const & arrowTarget, MapAnimation::Type type);
  void AnimateArrow(ArrowPose const & target, MapAnimation const * animation);
  void SetFollowAndRotate(m2::PointD const & userPos, m2::PointD const & pixelPos, double azimuth,
                          int preferredZoomLevel, std::optional<double> autoScale, bool isAnim,
                          std::optional<ArrowPose> const & arrowTarget);
  void SetAutoPerspective(bool isAutoPerspective);
  // The time left for the scale of a running animation that a request keeping the zoom changes to targetScale.
  double GetInheritedScaleTime(double targetScale) const;

  void ProcessTouch(TouchEvent const & touch);

  bool TouchDown(std::array<Touch, 2> const & touches);
  bool TouchMove(std::array<Touch, 2> const & touches);
  bool TouchCancel(std::array<Touch, 2> const & touches);
  bool TouchUp(std::array<Touch, 2> const & touches);
  void UpdateTouches(std::array<Touch, 2> const & touches);

  void BeginDrag(Touch const & t);
  void Drag(Touch const & t);
  // EndDrag returns false in case of kinetic moving after dragging has begun.
  bool EndDrag(Touch const & t, bool cancelled);

  void BeginScale(Touch const & t1, Touch const & t2);
  void Scale(Touch const & t1, Touch const & t2);
  void EndScale(Touch const & t1, Touch const & t2);

  void BeginTapDetector();
  void DetectShortTap(Touch const & touch);
  void DetectLongTap(Touch const & touch);
  bool DetectDoubleTap(Touch const & touch);
  void PerformDoubleTap(Touch const & touch);
  bool DetectForceTap(Touch const & touch);
  void EndTapDetector(Touch const & touch);
  void CancelTapDetector();

  void StartDoubleTapAndHold(Touch const & touch);
  void UpdateDoubleTapAndHold(Touch const & touch);
  void EndDoubleTapAndHold(Touch const & touch);

  void BeginTwoFingersTap(Touch const & t1, Touch const & t2);
  void EndTwoFingersTap();

  bool TryBeginFilter(Touch const & t);
  void EndFilter(Touch const & t);
  void CancelFilter(Touch const & t);

  bool CheckDrag(std::array<Touch, 2> const & touches, double threshold) const;

  using TEventsList = std::list<drape_ptr<UserEvent>>;
  TEventsList m_events;
  mutable std::mutex m_lock;

  m2::RectD m_visibleViewport;
  m2::PointD m_trackedCenter;
  bool m_needTrackCenter = false;

  Navigator m_navigator;
  base::Timer m_touchTimer;
  enum ERecognitionState
  {
    STATE_EMPTY,
    STATE_FILTER,
    STATE_TAP_DETECTION,
    STATE_WAIT_DOUBLE_TAP,
    STATE_TAP_TWO_FINGERS,
    STATE_WAIT_DOUBLE_TAP_HOLD,
    STATE_DOUBLE_TAP_HOLD,
    STATE_DRAG,
    STATE_SCALE
  } m_state;

  std::array<Touch, 2> m_touches;

  AnimationSystem & m_animationSystem;
  AnimationClock::time_point m_eventTime;

  ref_ptr<Listener> m_listener;

#ifdef DEBUG
  TTestBridge m_testFn;
#endif
  m2::PointD m_startDragOrg;
  std::array<m2::PointF, 2> m_twoFingersTouches;
  m2::PointD m_startDoubleTapAndHold;

  KineticScroller m_scroller;
  base::Timer m_kineticTimer;
  bool m_kineticScrollEnabled = true;
};
}  // namespace df
