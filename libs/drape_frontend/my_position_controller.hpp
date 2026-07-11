#pragma once

#include "drape_frontend/animation/animation.hpp"
#include "drape_frontend/drape_hints.hpp"
#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/my_position.hpp"

#include "drape/pointers.hpp"

#include "shaders/program_manager.hpp"

#include "platform/location.hpp"

#include "geometry/screenbase.hpp"

#include "base/timer.hpp"

#include <cstdint>
#include <functional>

namespace df
{
using TAnimationCreator = std::function<drape_ptr<Animation>(ref_ptr<Animation>)>;

class DrapeNotifier;

class MyPositionController
{
public:
  class Listener
  {
  public:
    virtual ~Listener() = default;
    virtual void PositionChanged(m2::PointD const & position, bool hasPosition) = 0;
    // Show map with center in "center" point and current zoom.
    virtual void ChangeModelView(m2::PointD const & center, int zoomLevel,
                                 TAnimationCreator const & parallelAnimCreator) = 0;
    // Change azimuth of current ModelView.
    virtual void ChangeModelView(double azimuth, TAnimationCreator const & parallelAnimCreator) = 0;
    // Somehow show map that "rect" will see.
    virtual void ChangeModelView(m2::RectD const & rect, TAnimationCreator const & parallelAnimCreator) = 0;
    // Show map where "usePos" (mercator) placed in "pxZero" on screen and map rotated around "userPos".
    virtual void ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero, int zoomLevel,
                                 Animation::TAction const & onFinishAction,
                                 TAnimationCreator const & parallelAnimCreator) = 0;
    virtual void ChangeModelView(double autoScale, m2::PointD const & userPos, double azimuth,
                                 m2::PointD const & pxZero, TAnimationCreator const & parallelAnimCreator) = 0;
  };

  struct Params
  {
    Params(location::EMyPositionMode initMode, double timeInBackground, Hints const & hints, bool isRoutingActive,
           bool isAutozoomEnabled, location::TMyPositionModeChanged && fn)
      : m_initMode(initMode)
      , m_timeInBackground(timeInBackground)
      , m_hints(hints)
      , m_isRoutingActive(isRoutingActive)
      , m_isAutozoomEnabled(isAutozoomEnabled)
      , m_myPositionModeCallback(std::move(fn))
    {}

    location::EMyPositionMode m_initMode;
    double m_timeInBackground;
    Hints m_hints;
    bool m_isRoutingActive;
    bool m_isAutozoomEnabled;
    location::TMyPositionModeChanged m_myPositionModeCallback;
  };

  MyPositionController(Params && params, ref_ptr<DrapeNotifier> notifier);

  void UpdatePosition();
  void OnUpdateScreen(ScreenBase const & screen);
  void SetVisibleViewport(m2::RectD const & rect);

  void SetListener(ref_ptr<Listener> listener);

  m2::PointD const & Position() const;
  double GetErrorRadius() const;
  double GetHorizontalAccuracy() const;

  bool IsModeHasPosition() const;

  void DragStarted();
  void DragEnded(m2::PointD const & distance);

  void ScaleStarted();
  void ScaleEnded();

  void Rotated();

  void Scrolled(m2::PointD const & distance);

  void ResetRoutingNotFollowTimer(bool blockTimer = false);
  void ResetBlockAutoZoomTimer();

  void CorrectScalePoint(m2::PointD & pt) const;
  void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const;
  void CorrectGlobalScalePoint(m2::PointD & pt) const;

  void SetRenderShape(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> texMng,
                      drape_ptr<MyPosition> && shape, Arrow3d::PreloadedData && preloadedData);
  void ResetRenderShape();

  void ActivateRouting(int zoomLevel, bool enableAutoZoom, bool isArrowGlued);
  void DeactivateRouting();

  void EnablePerspectiveInRouting(bool enablePerspective);
  void EnableAutoZoomInRouting(bool enableAutoZoom);

  void StopLocationFollow();
  void NextMode(ScreenBase const & screen);
  void LoseLocation();
  location::EMyPositionMode GetCurrentMode() const;

  void OnEnterForeground(double backgroundTime);
  void OnEnterBackground();

  void OnCompassTapped();
  void OnLocationUpdate(location::GpsInfo const & info, bool isNavigable, ScreenBase const & screen);
  void OnCompassUpdate(location::CompassInfo const & info, ScreenBase const & screen);

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, FrameValues const & frameValues);

  bool IsRotationAvailable() const { return m_isDirectionAssigned; }
  bool IsInRouting() const { return m_isInRouting; }
  bool IsRouteFollowingActive() const;
  bool IsModeChangeViewport() const;

  bool IsWaitingForLocation() const;
  m2::PointD GetDrawablePosition();
  void UpdateRoutingOffsetY(bool useDefault, int offsetY);

private:
  // The state is split into three independent axes: whether a position is available, whether
  // the camera follows the user and how a following camera is rotated. The platform-facing
  // location::EMyPositionMode is a projection of them, derived in GetCurrentMode().
  enum class PositionStatus
  {
    Stopped,    // Location updates are turned off.
    Acquiring,  // Waiting for a location fix.
    Available   // A position is available — from a location fix, or claimed when following starts.
  };

  enum class CameraTracking
  {
    Free,   // The user controls the viewport.
    Follow  // The camera tracks the user position.
  };

  enum class CameraRotation
  {
    Fixed,       // The map keeps its current azimuth.
    DirectionUp  // The map rotates after the movement/compass direction.
  };

  // Named state transitions: each one updates only the axes it owns, then notifies the platforms.
  // Changing the position status leaves the tracking/rotation axes as they were; reacquisition
  // re-derives the rotation (fixed outside routing, the preferred one in routing) rather than
  // restoring the pre-loss rotation.
  void SetPositionStatus(PositionStatus status);
  void StartFollowing(CameraRotation rotation);
  void StopFollowing();
  void NotifyModeChanged(location::EMyPositionMode oldMode);

  bool IsFollowing() const;
  bool IsFollowingDirectionUp() const;

  void SetDirection(double bearing);

  void ChangeModelView(m2::PointD const & center, int zoomLevel);
  void ChangeModelView(double azimuth);
  void ChangeModelView(m2::RectD const & rect);
  void ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero, int zoomLevel,
                       Animation::TAction const & onFinishAction = nullptr);
  void ChangeModelView(double autoScale, m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero);

  void UpdateViewport(int zoomLevel);
  bool UpdateViewportWithAutoZoom();
  m2::PointD GetRotationPixelCenter() const;
  m2::PointD GetRoutingRotationPixelCenter() const;

  double GetDrawableAzimut();
  void CreateAnim(m2::PointD const & oldPos, double oldAzimut, ScreenBase const & screen);

  bool AlmostCurrentPosition(m2::PointD const & pos) const;
  bool AlmostCurrentAzimut(double azimut) const;

  void CheckNotFollowRouting();
  void CheckBlockAutoZoom();
  void CheckUpdateLocation();

  ref_ptr<DrapeNotifier> m_notifier;

  PositionStatus m_positionStatus = PositionStatus::Acquiring;
  CameraTracking m_tracking = CameraTracking::Free;
  CameraRotation m_rotation = CameraRotation::Fixed;
  // The mode to adopt when the very first location fix of the session arrives;
  // it is consumed exactly once in OnLocationUpdate().
  location::EMyPositionMode m_desiredInitMode;
  location::TMyPositionModeChanged m_modeChangeCallback;
  Hints m_hints;

  bool m_isInRouting = false;
  bool m_isArrowGluedInRouting = false;

  bool m_needBlockAnimation;
  bool m_wasRotationInScaling;

  drape_ptr<MyPosition> m_shape;
  ref_ptr<Listener> m_listener;

  double m_errorRadius;  // error radius in mercator.
  double m_horizontalAccuracy;
  m2::PointD m_position;  // position in mercator.
  double m_drawDirection;
  m2::PointD m_oldPosition;  // position in mercator.
  double m_oldDrawDirection;

  bool m_enablePerspectiveInRouting;
  bool m_enableAutoZoomInRouting;
  double m_autoScale2d;
  double m_autoScale3d;

  base::Timer m_lastGPSBearingTimer;
  base::Timer m_routingNotFollowTimer;
  bool m_blockRoutingNotFollowTimer = false;
  base::Timer m_blockAutoZoomTimer;
  base::Timer m_updateLocationTimer;
  double m_lastLocationTimestamp;

  m2::RectD m_pixelRect;
  m2::RectD m_visiblePixelRect;
  double m_positionRoutingOffsetY;

  bool m_isDirtyViewport;
  bool m_isDirtyAutoZoom;
  bool m_isPendingAnimation;

  TAnimationCreator m_animCreator;

  bool m_isPositionAssigned;
  bool m_isDirectionAssigned;
  bool m_isCompassAvailable;

  bool m_positionIsObsolete;
  bool m_needBlockAutoZoom;

  uint64_t m_routingNotFollowNotifyId;
  uint64_t m_blockAutoZoomNotifyId;
  uint64_t m_updateLocationNotifyId;
};
}  // namespace df
