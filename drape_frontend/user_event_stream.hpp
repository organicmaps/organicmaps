#pragma once

#include "drape_frontend/kinetic_scroller.hpp"
#include "drape_frontend/navigator.hpp"
#include "drape_frontend/animation/model_view_animation.hpp"
#include "drape_frontend/animation/perspective_animation.hpp"

#include "drape/pointers.hpp"

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
  int64_t m_id = -1; // if id == -1 then touch is invalid
  float m_force = 0.0; // relative force of touch [0.0 - 1.0]
};

struct TouchEvent
{
  static uint8_t const INVALID_MASKED_POINTER;

  TouchEvent()
    : m_type(TOUCH_CANCEL)
    , m_timeStamp(my::Timer::LocalTime())
    , m_pointersMask(0xFFFF)
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
  array<Touch, 2> m_touches; // array of all touches
  double m_timeStamp; // seconds

  void PrepareTouches(array<Touch, 2> const & previousToches);

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

private:
  void Swap();
  uint16_t m_pointersMask;
};

struct ScaleEvent
{
  ScaleEvent(double factor, m2::PointD const & pxPoint, bool isAnim)
    : m_factor(factor)
    , m_pxPoint(pxPoint)
    , m_isAnim(isAnim)
  {
  }

  double m_factor;
  m2::PointD m_pxPoint;
  bool m_isAnim;
};

struct SetCenterEvent
{
  SetCenterEvent(m2::PointD const & center, int zoom, bool isAnim)
    : m_center(center)
    , m_zoom(zoom)
    , m_isAnim(isAnim)
  {
  }

  m2::PointD m_center; // center point in mercator
  int m_zoom; // if zoom == -1, then zoom level will'n change
  bool m_isAnim;
};

struct SetRectEvent
{
  SetRectEvent(m2::RectD const & rect, bool rotate, int zoom, bool isAnim)
    : m_rect(rect)
    , m_applyRotation(rotate)
    , m_zoom(zoom)
    , m_isAnim(isAnim)
  {
  }

  m2::RectD m_rect; // destination mercator rect
  bool m_applyRotation; // if true, current rotation will be apply to m_rect
  int m_zoom; // if zoom == -1, then zoom level will'n change
  bool m_isAnim;
};

struct SetAnyRectEvent
{
  SetAnyRectEvent(m2::AnyRectD const & rect, bool isAnim)
    : m_rect(rect)
    , m_isAnim(isAnim)
  {}

  m2::AnyRectD m_rect;  // destination mercator rect
  bool m_isAnim;
};

struct FollowAndRotateEvent
{
  FollowAndRotateEvent(m2::PointD const & userPos, m2::PointD const & pixelZero,
                       double azimuth, int preferredZoomLevel, bool isAnim)
    : m_userPos(userPos)
    , m_pixelZero(pixelZero)
    , m_azimuth(azimuth)
    , m_preferredZoomLevel(preferredZoomLevel)
    , m_isAnim(isAnim)
  {}

  m2::PointD m_userPos;
  m2::PointD m_pixelZero;
  double m_azimuth;
  int m_preferredZoomLevel;
  bool m_isAnim;
};

struct EnablePerspectiveEvent
{
  EnablePerspectiveEvent(double rotationAngle, double angleFOV,
                         bool isAnim, bool immediatelyStart)
    : m_isAnim(isAnim)
    , m_immediatelyStart(immediatelyStart)
    , m_rotationAngle(rotationAngle)
    , m_angleFOV(angleFOV)
  {}

  bool m_isAnim;
  bool m_immediatelyStart;
  double m_rotationAngle;
  double m_angleFOV;
};

struct DisablePerspectiveEvent
{
  DisablePerspectiveEvent() {}
};

struct SwitchViewModeEvent
{
  SwitchViewModeEvent(bool to2d): m_to2d(to2d) {}

  bool m_to2d;
};

struct RotateEvent
{
  RotateEvent(double targetAzimut) : m_targetAzimut(targetAzimut) {}

  double m_targetAzimut;
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
    EVENT_RESIZE,
    EVENT_ROTATE,
    EVENT_FOLLOW_AND_ROTATE,
    EVENT_ENABLE_PERSPECTIVE,
    EVENT_DISABLE_PERSPECTIVE,
    EVENT_SWITCH_VIEW_MODE
  };

  UserEvent(TouchEvent const & e) : m_type(EVENT_TOUCH) { m_touchEvent = e; }
  UserEvent(ScaleEvent const & e) : m_type(EVENT_SCALE) { m_scaleEvent = e; }
  UserEvent(SetCenterEvent const & e) : m_type(EVENT_SET_CENTER) { m_centerEvent = e; }
  UserEvent(SetRectEvent const & e) : m_type(EVENT_SET_RECT) { m_rectEvent = e; }
  UserEvent(SetAnyRectEvent const & e) : m_type(EVENT_SET_ANY_RECT) { m_anyRect = e; }
  UserEvent(ResizeEvent const & e) : m_type(EVENT_RESIZE) { m_resize = e; }
  UserEvent(RotateEvent const & e) : m_type(EVENT_ROTATE) { m_rotate = e; }
  UserEvent(FollowAndRotateEvent const & e) : m_type(EVENT_FOLLOW_AND_ROTATE) { m_followAndRotate = e; }
  UserEvent(EnablePerspectiveEvent const & e) : m_type(EVENT_ENABLE_PERSPECTIVE) { m_enable3dMode = e; }
  UserEvent(DisablePerspectiveEvent const & e) : m_type(EVENT_DISABLE_PERSPECTIVE) { m_disable3dMode = e; }
  UserEvent(SwitchViewModeEvent const & e) : m_type(EVENT_SWITCH_VIEW_MODE) { m_switchViewMode = e; }

  EEventType m_type;
  union
  {
    TouchEvent m_touchEvent;
    ScaleEvent m_scaleEvent;
    SetCenterEvent m_centerEvent;
    SetRectEvent m_rectEvent;
    SetAnyRectEvent m_anyRect;
    ResizeEvent m_resize;
    RotateEvent m_rotate;
    FollowAndRotateEvent m_followAndRotate;
    EnablePerspectiveEvent m_enable3dMode;
    DisablePerspectiveEvent m_disable3dMode;
    SwitchViewModeEvent m_switchViewMode;
  };
};

class UserEventStream
{
public:
  class Listener
  {
  public:
    virtual ~Listener() {}

    virtual void OnTap(m2::PointD const & pt, bool isLong) = 0;
    virtual void OnForceTap(m2::PointD const & pt) = 0;
    virtual void OnDoubleTap(m2::PointD const & pt) = 0;
    virtual void OnTwoFingersTap() = 0;
    virtual bool OnSingleTouchFiltrate(m2::PointD const & pt, TouchEvent::ETouchType type) = 0;
    virtual void OnDragStarted() = 0;
    virtual void OnDragEnded(m2::PointD const & distance) = 0;

    virtual void OnScaleStarted() = 0;
    virtual void OnRotated() = 0;
    virtual void CorrectScalePoint(m2::PointD & pt) const = 0;
    virtual void CorrectGlobalScalePoint(m2::PointD & pt) const = 0;
    virtual void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const = 0;
    virtual void OnScaleEnded() = 0;

    virtual void OnAnimationStarted(ref_ptr<BaseModelViewAnimation> anim) = 0;
  };

  UserEventStream(TIsCountryLoaded const & fn);
  void AddEvent(UserEvent const & event);
  ScreenBase const & ProcessEvents(bool & modelViewChange, bool & viewportChanged);
  ScreenBase const & GetCurrentScreen() const;

  m2::AnyRectD GetTargetRect() const;
  bool IsInUserAction() const;
  bool IsInPerspectiveAnimation() const;
  bool IsWaitingForActionCompletion() const;

  static bool IsScaleAllowableIn3d(int scale);

  void SetListener(ref_ptr<Listener> listener) { m_listener = listener; }

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

  using TTestBridge = function<void (char const * action)>;
  void SetTestBridge(TTestBridge const & fn) { m_testFn = fn; }
#endif

private:
  using TAnimationCreator = function<void(m2::AnyRectD const &, m2::AnyRectD const &, double, double, double)>;
  bool SetScale(m2::PointD const & pxScaleCenter, double factor, bool isAnim);
  bool SetCenter(m2::PointD const & center, int zoom, bool isAnim);
  bool SetRect(m2::RectD rect, int zoom, bool applyRotation, bool isAnim);
  bool SetRect(m2::AnyRectD const & rect, bool isAnim);
  bool SetRect(m2::AnyRectD const & rect, bool isAnim, TAnimationCreator const & animCreator);
  bool SetFollowAndRotate(m2::PointD const & userPos, m2::PointD const & pixelPos,
                          double azimuth, int preferredZoomLevel, bool isAnim);

  bool FilterEventWhile3dAnimation(UserEvent::EEventType type) const;
  void SetEnable3dMode(double maxRotationAngle, double angleFOV, bool isAnim, bool & viewportChanged);
  void SetDisable3dModeAnimation();

  m2::AnyRectD GetCurrentRect() const;

  bool ProcessTouch(TouchEvent const & touch);

  bool TouchDown(array<Touch, 2> const & touches);
  bool TouchMove(array<Touch, 2> const & touches, double timestamp);
  bool TouchCancel(array<Touch, 2> const & touches);
  bool TouchUp(array<Touch, 2> const & touches);
  void UpdateTouches(array<Touch, 2> const & touches);

  void BeginDrag(Touch const & t, double timestamp);
  void Drag(Touch const & t, double timestamp);
  // EndDrag returns false in case of kinetic moving after dragging has begun.
  bool EndDrag(Touch const & t, bool cancelled);

  void BeginScale(Touch const & t1, Touch const & t2);
  void Scale(Touch const & t1, Touch const & t2);
  void EndScale(Touch const & t1, Touch const & t2);

  void BeginTapDetector();
  void DetectShortTap(Touch const & touch);
  void DetectLongTap(Touch const & touch);
  bool DetectDoubleTap(Touch const & touch);
  bool DetectForceTap(Touch const & touch);
  void EndTapDetector(Touch const & touch);
  void CancelTapDetector();

  void BeginTwoFingersTap(Touch const & t1, Touch const & t2);
  void EndTwoFingersTap();

  bool TryBeginFilter(Touch const & t);
  void EndFilter(Touch const & t);
  void CancelFilter(Touch const & t);

  void ResetCurrentAnimation(bool finishAnimation = false);

private:
  TIsCountryLoaded m_isCountryLoaded;

  list<UserEvent> m_events;
  mutable mutex m_lock;

  Navigator m_navigator;
  my::Timer m_touchTimer;
  enum ERecognitionState
  {
    STATE_EMPTY,
    STATE_FILTER,
    STATE_TAP_DETECTION,
    STATE_WAIT_DOUBLE_TAP,
    STATE_TAP_TWO_FINGERS,
    STATE_DRAG,
    STATE_SCALE
  } m_state;

  array<Touch, 2> m_touches;

  drape_ptr<BaseModelViewAnimation> m_animation;

  unique_ptr<PerspectiveAnimation> m_perspectiveAnimation;
  unique_ptr<UserEvent> m_pendingEvent;
  double m_discardedFOV = 0.0;
  double m_discardedAngle = 0.0;

  ref_ptr<Listener> m_listener;

#ifdef DEBUG
  TTestBridge m_testFn;
#endif
  m2::PointD m_startDragOrg;
  array<m2::PointF, 2> m_twoFingersTouches;

  KineticScroller m_scroller;
  my::Timer m_kineticTimer;
};

}
