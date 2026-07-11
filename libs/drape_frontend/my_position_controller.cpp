#include "drape_frontend/my_position_controller.hpp"

#include "drape_frontend/animation/arrow_animation.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/animation_utils.hpp"
#include "drape_frontend/drape_notifier.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <array>
#include <chrono>

namespace df
{
namespace
{
int const kPositionRoutingOffsetY = 104;

// https://t.me/OrganicMapsRu/88317
double const kMinSpeedThresholdMps = 0.7;  // for the pedestrian mode 2.5 km/h
/// @todo Should depend on the _previous_ avg speed (say for the last 5 minutes).
/// Bigger for cars (up to 30 seconds is ok, IMO) and lower for pedestrians.
double const kGpsBearingLifetimeSec = 3.0;

double const kMaxTimeInBackgroundSec = 60.0 * 60 * 30;  // 30 hours before starting detecting position again
double const kMaxNotFollowRoutingTimeSec = 20.0;
double const kMaxUpdateLocationInvervalSec = 30.0;
double const kMaxBlockAutoZoomTimeSec = 10.0;

int const kZoomThreshold = 10;
int const kMaxScaleZoomLevel = 16;
int const kDefaultAutoZoom = 16;
double const kUnknownAutoZoom = -1.0;

inline int GetZoomLevel(ScreenBase const & screen)
{
  return static_cast<int>(df::GetZoomLevel(screen.GetScale()));
}

int GetZoomLevel(ScreenBase const & screen, m2::PointD const & position, double errorRadius)
{
  ScreenBase s = screen;
  m2::PointD const size(errorRadius, errorRadius);
  s.SetFromRect(
      m2::AnyRectD(position, ang::Angle<double>(screen.GetAngle()), m2::RectD(position - size, position + size)));
  return GetZoomLevel(s);
}

inline double GetVisualScale()
{
  return df::VisualParams::Instance().GetVisualScale();
}

// Calculate zoom value in meters per pixel
double CalculateZoomBySpeed(double speedMpS, bool isPerspectiveAllowed)
{
  using TSpeedScale = std::pair<double, double>;
  static std::array<TSpeedScale, 6> const scales3d = {{
      {20.0, 0.25},
      {40.0, 0.75},
      {60.0, 1.50},
      {75.0, 2.50},
      {85.0, 3.75},
      {95.0, 6.00},
  }};

  static std::array<TSpeedScale, 6> const scales2d = {{
      {20.0, 0.70},
      {40.0, 1.25},
      {60.0, 2.25},
      {75.0, 3.00},
      {85.0, 3.75},
      {95.0, 6.00},
  }};

  std::array<TSpeedScale, 6> const & scales = isPerspectiveAllowed ? scales3d : scales2d;

  double constexpr kDefaultSpeedKmpH = 80.0;
  double const speedKmpH = speedMpS >= 0 ? measurement_utils::MpsToKmph(speedMpS) : kDefaultSpeedKmpH;

  size_t i = 0;
  for (size_t sz = scales.size(); i < sz; ++i)
    if (scales[i].first >= speedKmpH)
      break;

  double const vs = GetVisualScale();

  if (i == 0)
    return scales.front().second / vs;
  if (i == scales.size())
    return scales.back().second / vs;

  double const minSpeed = scales[i - 1].first;
  double const maxSpeed = scales[i].first;
  double const k = (speedKmpH - minSpeed) / (maxSpeed - minSpeed);

  double const minScale = scales[i - 1].second;
  double const maxScale = scales[i].second;
  double const zoom = minScale + k * (maxScale - minScale);

  return zoom / vs;
}

void ResetNotification(uint64_t & notifyId)
{
  notifyId = DrapeNotifier::kInvalidId;
}
}  // namespace

MyPositionController::MyPositionController(Params && params, ref_ptr<DrapeNotifier> notifier)
  : m_notifier(notifier)
  , m_modeChangeCallback(std::move(params.m_myPositionModeCallback))
  , m_hints(params.m_hints)
  , m_isInRouting(params.m_isRoutingActive)
  , m_routingOrientation(params.m_routingOrientation)
  , m_routingOrientationChangedCallback(std::move(params.m_routingOrientationChangedCallback))
  , m_needBlockAnimation(false)
  , m_wasRotationInScaling(false)
  , m_errorRadius(0.0)
  , m_horizontalAccuracy(0.0)
  , m_position(m2::PointD::Zero())
  , m_drawDirection(0.0)
  , m_oldPosition(m2::PointD::Zero())
  , m_oldDrawDirection(0.0)
  , m_enablePerspectiveInRouting(false)
  , m_enableAutoZoomInRouting(params.m_isAutozoomEnabled)
  , m_autoScale2d(GetScreenScale(kDefaultAutoZoom))
  , m_autoScale3d(m_autoScale2d)
  , m_lastGPSBearingTimer(false)
  , m_lastLocationTimestamp(0.0)
  , m_positionRoutingOffsetY(kPositionRoutingOffsetY * GetVisualScale())
  , m_isDirtyViewport(false)
  , m_isDirtyAutoZoom(false)
  , m_isPendingAnimation(false)
  , m_isPositionAssigned(false)
  , m_isDirectionAssigned(false)
  , m_isCompassAvailable(false)
  , m_positionIsObsolete(false)
  , m_needBlockAutoZoom(false)
  , m_routingNotFollowNotifyId(DrapeNotifier::kInvalidId)
  , m_blockAutoZoomNotifyId(DrapeNotifier::kInvalidId)
  , m_updateLocationNotifyId(DrapeNotifier::kInvalidId)
{
  using namespace location;

  if (m_hints.m_isLaunchByDeepLink)
  {
    m_desiredInitMode = NotFollow;
  }
  else if (m_hints.m_isFirstLaunch)
  {
    m_desiredInitMode = Follow;
  }
  else if (params.m_timeInBackground >= kMaxTimeInBackgroundSec)
  {
    m_desiredInitMode = Follow;
  }
  else
  {
    m_desiredInitMode = params.m_initMode;

    // Do not start position if we ended previous session without it.
    if (!m_isInRouting && m_desiredInitMode == NotFollowNoPosition)
      m_positionStatus = PositionStatus::Stopped;
  }

  // During active routing the camera follows the route (isRoutingActive is passed as
  // IsRoutingActive() && IsRoutingFollowing()), so no non-following init mode is coherent: restore
  // the preferred routing orientation and re-enable the location search if it had been turned off.
  if (m_isInRouting)
    m_desiredInitMode = GetPreferredRoutingMode();

  if (m_modeChangeCallback)
    m_modeChangeCallback(GetCurrentMode(), m_isInRouting);
}

location::EMyPositionMode MyPositionController::GetCurrentMode() const
{
  switch (m_positionStatus)
  {
  case PositionStatus::Stopped: return location::NotFollowNoPosition;
  case PositionStatus::Acquiring: return location::PendingPosition;
  case PositionStatus::Available:
    if (m_tracking == CameraTracking::Free)
      return location::NotFollow;
    return m_rotation == CameraRotation::DirectionUp ? location::FollowAndRotate : location::Follow;
  }
  UNREACHABLE();
}

void MyPositionController::SetPositionStatus(PositionStatus status)
{
  auto const oldMode = GetCurrentMode();
  m_positionStatus = status;
  NotifyModeChanged(oldMode);
}

void MyPositionController::StartFollowing(CameraRotation rotation)
{
  auto const oldMode = GetCurrentMode();
  // Following implies an available position, so claim one here: routing may start the following
  // camera even before the first location fix.
  m_positionStatus = PositionStatus::Available;
  m_tracking = CameraTracking::Follow;
  m_rotation = rotation;
  NotifyModeChanged(oldMode);
}

void MyPositionController::StopFollowing()
{
  auto const oldMode = GetCurrentMode();
  // A free camera still shows the user position.
  m_positionStatus = PositionStatus::Available;
  m_tracking = CameraTracking::Free;
  NotifyModeChanged(oldMode);
}

void MyPositionController::NotifyModeChanged(location::EMyPositionMode oldMode)
{
  auto const newMode = GetCurrentMode();
  if (m_isInRouting && oldMode != newMode && location::IsFollowingMode(newMode))
    ResetBlockAutoZoomTimer();

  if (m_modeChangeCallback)
    m_modeChangeCallback(newMode, m_isInRouting);
  if (m_listener)
    m_listener->MyPositionModeChanged(newMode, m_isInRouting);
}

void MyPositionController::SetRoutingOrientation(location::MapOrientation orientation)
{
  CHECK(m_isInRouting, ());
  if (m_routingOrientation == orientation)
    return;

  m_routingOrientation = orientation;
  if (m_routingOrientationChangedCallback)
    m_routingOrientationChangedCallback(orientation);
}

location::EMyPositionMode MyPositionController::GetPreferredRoutingMode() const
{
  return location::ToPositionMode(m_routingOrientation);
}

MyPositionController::CameraRotation MyPositionController::GetPreferredRoutingRotation() const
{
  return m_routingOrientation == location::MapOrientation::NorthUp ? CameraRotation::Fixed
                                                                   : CameraRotation::DirectionUp;
}

void MyPositionController::UpdateRoutingViewport(int zoomLevel, Animation::TAction const & onFinishAction)
{
  CHECK(m_isInRouting, ());
  CHECK(IsFollowing(), ());

  if (m_routingOrientation == location::MapOrientation::NorthUp)
  {
    // Request the azimuth explicitly: the center-only ChangeModelView() overload
    // would keep the current map rotation instead of north-up.
    ChangeModelView(m_position, 0.0 /* north up */, m_visiblePixelRect.Center(), zoomLevel, onFinishAction);
    return;
  }

  ChangeModelView(m_position, m_isDirectionAssigned ? m_drawDirection : 0.0, GetRoutingRotationPixelCenter(), zoomLevel,
                  onFinishAction);
}

bool MyPositionController::ShouldEnablePerspectiveInRouting() const
{
  // The perspective policy depends on the chosen routing orientation only: temporary states
  // like an on-going location search or a not-follow pan must not flatten the camera.
  return m_isInRouting && m_enablePerspectiveInRouting && m_routingOrientation == location::MapOrientation::HeadingUp;
}

bool MyPositionController::IsFollowing() const
{
  return m_positionStatus == PositionStatus::Available && m_tracking == CameraTracking::Follow;
}

bool MyPositionController::IsFollowingDirectionUp() const
{
  return IsFollowing() && m_rotation == CameraRotation::DirectionUp;
}

void MyPositionController::UpdatePosition()
{
  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::OnUpdateScreen(ScreenBase const & screen)
{
  m_pixelRect = screen.PixelRectIn3d();
  if (m_visiblePixelRect.IsEmptyInterior())
    SetVisibleViewport(m_pixelRect);
}

void MyPositionController::SetVisibleViewport(m2::RectD const & rect)
{
  m_visiblePixelRect = rect;
}

void MyPositionController::SetListener(ref_ptr<MyPositionController::Listener> listener)
{
  m_listener = listener;
}

m2::PointD const & MyPositionController::Position() const
{
  return m_position;
}

double MyPositionController::GetErrorRadius() const
{
  return m_errorRadius;
}

double MyPositionController::GetHorizontalAccuracy() const
{
  return m_horizontalAccuracy;
}

bool MyPositionController::IsModeChangeViewport() const
{
  return IsFollowing();
}

bool MyPositionController::IsModeHasPosition() const
{
  return m_positionStatus == PositionStatus::Available;
}

void MyPositionController::DragStarted()
{
  m_needBlockAnimation = true;
}

void MyPositionController::DragEnded(m2::PointD const & distance)
{
  float const kBindingDistance = 0.1f;
  m_needBlockAnimation = false;
  if (distance.Length() > kBindingDistance * std::min(m_pixelRect.SizeX(), m_pixelRect.SizeY()))
    StopLocationFollow();

  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::ScaleStarted()
{
  m_needBlockAnimation = true;
  ResetBlockAutoZoomTimer();
}

void MyPositionController::ScaleEnded()
{
  m_needBlockAnimation = false;
  ResetBlockAutoZoomTimer();
  if (m_wasRotationInScaling)
  {
    m_wasRotationInScaling = false;
    StopLocationFollow();
  }

  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::Rotated()
{
  if (IsFollowingDirectionUp())
    m_wasRotationInScaling = true;
}

void MyPositionController::Scrolled(m2::PointD const & distance)
{
  if (m_positionStatus == PositionStatus::Acquiring)
    return;

  if (distance.Length() > 0)
    StopLocationFollow();

  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::ResetRoutingNotFollowTimer(bool blockTimer)
{
  if (m_isInRouting)
  {
    m_routingNotFollowTimer.Reset();
    m_blockRoutingNotFollowTimer = blockTimer;
    ResetNotification(m_routingNotFollowNotifyId);
  }
}

void MyPositionController::ResetBlockAutoZoomTimer()
{
  if (m_isInRouting && m_enableAutoZoomInRouting)
  {
    m_needBlockAutoZoom = true;
    m_blockAutoZoomTimer.Reset();
    ResetNotification(m_blockAutoZoomNotifyId);
  }
}

void MyPositionController::CorrectScalePoint(m2::PointD & pt) const
{
  if (IsModeChangeViewport())
    pt = GetRotationPixelCenter();
}

void MyPositionController::CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const
{
  if (IsModeChangeViewport())
  {
    m2::PointD const oldPt1(pt1);
    pt1 = GetRotationPixelCenter();
    pt2 = pt2 - oldPt1 + pt1;
  }
}

void MyPositionController::CorrectGlobalScalePoint(m2::PointD & pt) const
{
  if (IsModeChangeViewport())
    pt = m_position;
}

void MyPositionController::SetRenderShape(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> texMng,
                                          drape_ptr<MyPosition> && shape, Arrow3d::PreloadedData && preloadedData)
{
  m_shape = std::move(shape);
  if (!m_shape->InitArrow(context, texMng, std::move(preloadedData)))
  {
    m_shape.reset();
    LOG(LERROR, ("Invalid Arrow3D mesh."));
  }
}

void MyPositionController::ResetRenderShape()
{
  m_shape.reset();
}

void MyPositionController::NextMode(ScreenBase const & screen)
{
  // When the app is awaiting location (indicator is active) and the user presses on the indicator, location updates
  // will be stopped and goes into Stopped state. The next press on the indicator will start location updates again.
  if (IsWaitingForLocation())
  {
    // Preserve the routing orientation that should be restored after location becomes available.
    m_desiredInitMode = m_isInRouting ? GetPreferredRoutingMode() : location::Follow;
    SetPositionStatus(PositionStatus::Stopped);
    return;
  }

  // Start looking for location.
  if (m_positionStatus == PositionStatus::Stopped)
  {
    SetPositionStatus(PositionStatus::Acquiring);

    if (!m_isPositionAssigned)
    {
      // This is the first user location request (button touch) after controller's initialization
      // with some previous not Follow state. The new mode will be Follow to center on the position.
      m_desiredInitMode = m_isInRouting ? GetPreferredRoutingMode() : location::Follow;
    }
    return;
  }

  // Calculate preferred zoom level.
  int const currentZoom = GetZoomLevel(screen);
  int preferredZoomLevel = kDoNotChangeZoom;
  if (currentZoom < kZoomThreshold)
    preferredZoomLevel = std::min(GetZoomLevel(screen, m_position, m_errorRadius), kMaxScaleZoomLevel);

  // A free camera returns to following: the preferred orientation in routing, fixed rotation otherwise.
  if (m_tracking == CameraTracking::Free)
  {
    StartFollowing(m_isInRouting ? GetPreferredRoutingRotation() : CameraRotation::Fixed);
    UpdateViewport(preferredZoomLevel);
    return;
  }

  // From fixed rotation we transit to direction-up if compass is available or
  // routing is enabled.
  if (m_rotation == CameraRotation::Fixed)
  {
    if (IsRotationAvailable() || m_isInRouting)
    {
      if (m_isInRouting)
        SetRoutingOrientation(location::MapOrientation::HeadingUp);
      StartFollowing(CameraRotation::DirectionUp);
      UpdateViewport(preferredZoomLevel);
    }
    return;
  }

  // From direction-up we return to fixed rotation and reset the map to north-up.
  if (m_isInRouting && screen.isPerspective())
    preferredZoomLevel = static_cast<int>(GetZoomLevel(ScreenBase::GetStartPerspectiveScale() * 1.1));
  if (m_isInRouting)
    SetRoutingOrientation(location::MapOrientation::NorthUp);
  StartFollowing(CameraRotation::Fixed);
  ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), preferredZoomLevel);
}

void MyPositionController::OnLocationUpdate(location::GpsInfo const & info, bool isNavigable, ScreenBase const & screen)
{
  m2::PointD const oldPos = GetDrawablePosition();
  double const oldAzimut = GetDrawableAzimut();

  m2::RectD const rect = mercator::MetersToXY(info.m_longitude, info.m_latitude, info.m_horizontalAccuracy);
  // Use FromLatLon instead of rect.Center() since in case of large info.m_horizontalAccuracy
  // there is significant difference between the real location and the estimated one.
  m_position = mercator::FromLatLon(info.m_latitude, info.m_longitude);
  m_errorRadius = rect.SizeX() * 0.5;
  m_horizontalAccuracy = info.m_horizontalAccuracy;

  if (info.m_speed > 0.0)
  {
    double const mercatorPerMeter = m_errorRadius / info.m_horizontalAccuracy;
    m_autoScale2d = mercatorPerMeter * CalculateZoomBySpeed(info.m_speed, false /* isPerspectiveAllowed */);
    m_autoScale3d = mercatorPerMeter * CalculateZoomBySpeed(info.m_speed, true /* isPerspectiveAllowed */);
  }
  else
  {
    m_autoScale2d = m_autoScale3d = kUnknownAutoZoom;
  }

  // Sets direction based on GPS if:
  // 1. Compass is not available.
  // 2. Direction must be glued to the route during routing (route-corrected angle is set only in
  // OnLocationUpdate(): in OnCompassUpdate() the angle always has the original value.
  // 3. Device is moving faster then pedestrian.
  bool const isMovingFast = info.HasSpeed() && info.m_speed > kMinSpeedThresholdMps;
  bool const glueArrowInRouting = isNavigable && m_isArrowGluedInRouting;

  if ((!m_isCompassAvailable || glueArrowInRouting || isMovingFast) && info.HasBearing())
  {
    SetDirection(math::DegToRad(info.m_bearing));
    m_lastGPSBearingTimer.Reset();
  }

  if (m_isPositionAssigned && (!AlmostCurrentPosition(oldPos) || !AlmostCurrentAzimut(oldAzimut)))
  {
    CreateAnim(oldPos, oldAzimut, screen);
    m_isDirtyViewport = true;
  }

  // Assume that every new position is fresh enough. We can't make some straightforward filtering here
  // like comparing system_clock::now().time_since_epoch() and info.m_timestamp, because can't rely
  // on valid time settings on endpoint device.
  m_positionIsObsolete = false;

  if (!m_isPositionAssigned)
  {
    // The very first fix of the session applies the mode the controller was initialized with.
    switch (m_desiredInitMode)
    {
    case location::Follow: StartFollowing(CameraRotation::Fixed); break;
    case location::FollowAndRotate: StartFollowing(CameraRotation::DirectionUp); break;
    case location::NotFollow: StopFollowing(); break;
    case location::NotFollowNoPosition: SetPositionStatus(PositionStatus::Stopped); break;
    case location::PendingPosition: SetPositionStatus(PositionStatus::Acquiring); break;
    }

    if (!m_hints.m_isFirstLaunch || !AnimationSystem::Instance().AnimationExists(Animation::Object::MapPlane))
    {
      if (m_isInRouting && IsFollowing())
        UpdateRoutingViewport(kDoNotChangeZoom);
      else if (IsFollowingDirectionUp())
        ChangeModelView(m_position, m_drawDirection, m_visiblePixelRect.Center(), kDoNotChangeZoom);
      else if (IsFollowing())
        ChangeModelView(m_position, kDoNotChangeZoom);
    }
  }
  else if (m_positionStatus == PositionStatus::Acquiring)
  {
    if (m_isInRouting)
    {
      StartFollowing(GetPreferredRoutingRotation());
      UpdateViewport(kMaxScaleZoomLevel);
    }
    else
    {
      StartFollowing(CameraRotation::Fixed);
      if (m_hints.m_isFirstLaunch)
      {
        if (!AnimationSystem::Instance().AnimationExists(Animation::Object::MapPlane))
          ChangeModelView(m_position, kDoNotChangeZoom);
      }
      else if (GetZoomLevel(screen, m_position, m_errorRadius) <= kMaxScaleZoomLevel)
      {
        m2::PointD const size(m_errorRadius, m_errorRadius);
        ChangeModelView(m2::RectD(m_position - size, m_position + size));
      }
      else
      {
        ChangeModelView(m_position, kMaxScaleZoomLevel);
      }
    }
  }
  else if (m_positionStatus == PositionStatus::Stopped)
  {
    if (m_isInRouting)
    {
      StartFollowing(GetPreferredRoutingRotation());
      UpdateViewport(kMaxScaleZoomLevel);
    }
    else
    {
      // Here we silently get the position and stop following.
      StopFollowing();
    }
  }

  m_isPositionAssigned = true;

  if (m_listener != nullptr)
    m_listener->PositionChanged(Position(), IsModeHasPosition());

  if (fabs(m_lastLocationTimestamp - info.m_timestamp) > 1.0E-5)
  {
    m_lastLocationTimestamp = info.m_timestamp;
    m_updateLocationTimer.Reset();
    ResetNotification(m_updateLocationNotifyId);
  }
}

void MyPositionController::LoseLocation()
{
  if (m_positionStatus == PositionStatus::Stopped)
    return;

  // Keep searching for the position (Acquiring) when the camera was following, so following can
  // resume once a fix returns; otherwise turn updates off.
  if (IsFollowing())
    SetPositionStatus(PositionStatus::Acquiring);
  else
    SetPositionStatus(PositionStatus::Stopped);

  if (m_listener != nullptr)
    m_listener->PositionChanged(Position(), false /* hasPosition */);
}

void MyPositionController::OnCompassUpdate(location::CompassInfo const & info, ScreenBase const & screen)
{
  m_isCompassAvailable = true;

  if (m_isArrowGluedInRouting && IsInRouting())
    return;

  if (m_lastGPSBearingTimer.ElapsedSeconds() < kGpsBearingLifetimeSec)
    return;

  double const oldAzimut = GetDrawableAzimut();

  SetDirection(info.m_bearing);

  if (m_isPositionAssigned && IsFollowingDirectionUp() && !AlmostCurrentAzimut(oldAzimut))
  {
    CreateAnim(GetDrawablePosition(), oldAzimut, screen);
    m_isDirtyViewport = true;
  }
}

bool MyPositionController::UpdateViewportWithAutoZoom()
{
  if (!m_isInRouting || !m_enableAutoZoomInRouting || m_needBlockAutoZoom)
    return false;

  if (IsFollowingDirectionUp())
  {
    double const autoScale = ShouldEnablePerspectiveInRouting() ? m_autoScale3d : m_autoScale2d;
    if (autoScale > 0.0)
    {
      ChangeModelView(autoScale, m_position, m_drawDirection, GetRoutingRotationPixelCenter());
      return true;
    }
  }
  else if (IsFollowing() && m_autoScale2d > 0.0)
  {
    // North-up navigation: use the 2D speed scale, keep the north-up azimuth and center the user.
    ChangeModelView(m_autoScale2d, m_position, 0.0 /* north up */, m_visiblePixelRect.Center());
    return true;
  }
  return false;
}

void MyPositionController::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                  ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  CheckNotFollowRouting();

  if (m_shape != nullptr && IsModeHasPosition())
  {
    CheckBlockAutoZoom();
    CheckUpdateLocation();

    if ((m_isDirtyViewport || m_isDirtyAutoZoom) && !m_needBlockAnimation)
    {
      if (!UpdateViewportWithAutoZoom() && m_isDirtyViewport)
        UpdateViewport(kDoNotChangeZoom);
      m_isDirtyViewport = false;
      m_isDirtyAutoZoom = false;
    }

    if (!IsModeChangeViewport())
      m_isPendingAnimation = false;

    /// @todo Put under !m_hints.m_screenshotMode?
    /// Why do we have 6 modifiers (and 6 variables inside), if better to make 1 function m_shape->Render(Params)?
    m_shape->SetPositionObsolete(m_positionIsObsolete);
    m_shape->SetPosition(m2::PointF(GetDrawablePosition()));
    m_shape->SetAzimuth(static_cast<float>(GetDrawableAzimut()));
    m_shape->SetIsValidAzimuth(IsRotationAvailable());
    m_shape->SetAccuracy(static_cast<float>(m_errorRadius));
    m_shape->SetRoutingMode(IsInRouting());

    if (!m_hints.m_screenshotMode)
    {
      m_shape->RenderAccuracy(context, mng, screen, zoomLevel, frameValues);
      m_shape->RenderMyPosition(context, mng, screen, zoomLevel, frameValues);
    }
  }
}

bool MyPositionController::IsRouteFollowingActive() const
{
  // North-up navigation follows the route in the fixed-rotation camera, so any following
  // camera counts, not only the direction-up one.
  return IsInRouting() && IsFollowing();
}

bool MyPositionController::AlmostCurrentPosition(m2::PointD const & pos) const
{
  double constexpr kPositionEqualityDelta = 1e-5;
  return pos.EqualDxDy(m_position, kPositionEqualityDelta);
}

bool MyPositionController::AlmostCurrentAzimut(double azimut) const
{
  double constexpr kDirectionEqualityDelta = 1e-3;
  return AlmostEqualAbs(azimut, m_drawDirection, kDirectionEqualityDelta);
}

void MyPositionController::SetDirection(double bearing)
{
  m_drawDirection = bearing;
  m_isDirectionAssigned = true;
}

bool MyPositionController::IsWaitingForLocation() const
{
  if (m_positionStatus == PositionStatus::Stopped)
    return false;

  if (!m_isPositionAssigned)
    return true;

  return m_positionStatus == PositionStatus::Acquiring;
}

void MyPositionController::StopLocationFollow()
{
  if (IsFollowing())
    StopFollowing();
  m_desiredInitMode = location::NotFollow;

  ResetRoutingNotFollowTimer();
}

void MyPositionController::OnEnterForeground(double backgroundTime)
{
  // Handle the case when the app was in the background for a long time and the user is opening the app.
  if (backgroundTime >= kMaxTimeInBackgroundSec)
  {
    // When location was active during previous session the app will try to follow the user.
    if (m_positionStatus == PositionStatus::Available && m_tracking == CameraTracking::Free)
    {
      StartFollowing(m_isInRouting ? GetPreferredRoutingRotation() : CameraRotation::Fixed);
      UpdateViewport(kDoNotChangeZoom);
    }

    // When location was stopped by the user manually app will try to find position but without following.
    else if (m_positionStatus == PositionStatus::Stopped)
    {
      SetPositionStatus(PositionStatus::Acquiring);
    }
  }
}

void MyPositionController::OnEnterBackground() {}

void MyPositionController::OnCompassTapped()
{
  if (IsFollowingDirectionUp())
  {
    if (m_isInRouting)
      SetRoutingOrientation(location::MapOrientation::NorthUp);
    StartFollowing(CameraRotation::Fixed);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), kDoNotChangeZoom);
  }
  else
  {
    ChangeModelView(0.0);
  }
}

void MyPositionController::ChangeModelView(m2::PointD const & center, int zoomLevel)
{
  if (m_listener)
    m_listener->ChangeModelView(center, zoomLevel, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(double azimuth)
{
  if (m_listener)
    m_listener->ChangeModelView(azimuth, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(m2::RectD const & rect)
{
  if (m_listener)
    m_listener->ChangeModelView(rect, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero,
                                           int zoomLevel, Animation::TAction const & onFinishAction)
{
  if (m_listener)
    m_listener->ChangeModelView(userPos, azimuth, pxZero, zoomLevel, onFinishAction, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(double autoScale, m2::PointD const & userPos, double azimuth,
                                           m2::PointD const & pxZero)
{
  if (m_listener)
    m_listener->ChangeModelView(autoScale, userPos, azimuth, pxZero, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::UpdateViewport(int zoomLevel)
{
  if (IsWaitingForLocation())
    return;

  // Every routing-follow restoration goes through UpdateRoutingViewport() to apply
  // the chosen routing orientation.
  if (m_isInRouting && IsFollowing())
  {
    UpdateRoutingViewport(zoomLevel);
    return;
  }

  if (IsFollowingDirectionUp())
  {
    ChangeModelView(m_position, m_drawDirection, m_visiblePixelRect.Center(), zoomLevel);
  }
  else if (IsFollowing())
  {
    // Outside routing the map may retain a manually chosen fixed angle.
    ChangeModelView(m_position, zoomLevel);
  }
}

m2::PointD MyPositionController::GetRotationPixelCenter() const
{
  if (IsFollowingDirectionUp())
    return m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center();

  if (IsFollowing())
    return m_visiblePixelRect.Center();

  return m2::PointD::Zero();
}

m2::PointD MyPositionController::GetRoutingRotationPixelCenter() const
{
  return {m_visiblePixelRect.Center().x, m_visiblePixelRect.maxY() - m_positionRoutingOffsetY};
}

void MyPositionController::UpdateRoutingOffsetY(bool useDefault, int offsetY)
{
  double const vs = GetVisualScale();
  m_positionRoutingOffsetY = useDefault ? kPositionRoutingOffsetY * vs : offsetY + Arrow3d::GetMaxBottomSize() * vs;
}

m2::PointD MyPositionController::GetDrawablePosition()
{
  m2::PointD position;
  if (AnimationSystem::Instance().GetArrowPosition(position))
  {
    m_isPendingAnimation = false;
    return position;
  }

  if (m_isPendingAnimation)
    return m_oldPosition;

  return m_position;
}

double MyPositionController::GetDrawableAzimut()
{
  double angle;
  if (AnimationSystem::Instance().GetArrowAngle(angle))
  {
    m_isPendingAnimation = false;
    return angle;
  }

  if (m_isPendingAnimation)
    return m_oldDrawDirection;

  return m_drawDirection;
}

void MyPositionController::CreateAnim(m2::PointD const & oldPos, double oldAzimut, ScreenBase const & screen)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(oldPos, m_position, screen);
  double const rotateDuration = AngleInterpolator::GetRotateDuration(oldAzimut, m_drawDirection);
  if (df::IsAnimationAllowed(std::max(moveDuration, rotateDuration), screen))
  {
    if (IsModeChangeViewport())
    {
      m_animCreator = [this, moveDuration](ref_ptr<Animation> syncAnim) -> drape_ptr<Animation>
      {
        drape_ptr<Animation> anim = make_unique_dp<ArrowAnimation>(
            GetDrawablePosition(), m_position, syncAnim == nullptr ? moveDuration : syncAnim->GetDuration(),
            GetDrawableAzimut(), m_drawDirection);
        if (syncAnim != nullptr)
        {
          anim->SetMaxDuration(syncAnim->GetMaxDuration());
          anim->SetMinDuration(syncAnim->GetMinDuration());
        }
        return anim;
      };
      m_oldPosition = oldPos;
      m_oldDrawDirection = oldAzimut;
      m_isPendingAnimation = true;
    }
    else
    {
      AnimationSystem::Instance().CombineAnimation(
          make_unique_dp<ArrowAnimation>(oldPos, m_position, moveDuration, oldAzimut, m_drawDirection));
    }
  }
}

void MyPositionController::EnablePerspectiveInRouting(bool enablePerspective)
{
  m_enablePerspectiveInRouting = enablePerspective;
}

void MyPositionController::EnableAutoZoomInRouting(bool enableAutoZoom)
{
  m_enableAutoZoomInRouting = enableAutoZoom;
  ResetBlockAutoZoomTimer();
}

void MyPositionController::ActivateRouting(int zoomLevel, int zoomLevelIn3d, bool enableAutoZoom, bool isArrowGlued)
{
  if (!m_isInRouting)
  {
    m_isInRouting = true;
    m_isArrowGluedInRouting = isArrowGlued;
    m_enableAutoZoomInRouting = enableAutoZoom;
    // Preserve the routing orientation that should be restored after location becomes available.
    m_desiredInitMode = GetPreferredRoutingMode();

    StartFollowing(GetPreferredRoutingRotation());
    int const preferredZoomLevel = ShouldEnablePerspectiveInRouting() ? zoomLevelIn3d : zoomLevel;
    UpdateRoutingViewport(preferredZoomLevel, [this](ref_ptr<Animation> anim) { UpdateViewport(kDoNotChangeZoom); });
    ResetRoutingNotFollowTimer();
  }
}

void MyPositionController::DeactivateRouting()
{
  if (m_isInRouting)
  {
    m_isInRouting = false;
    m_isArrowGluedInRouting = false;

    m_isDirectionAssigned = m_isCompassAvailable && m_isDirectionAssigned;

    StartFollowing(CameraRotation::Fixed);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), kDoNotChangeZoom);
  }
}

// This code schedules the execution of checkFunction on FR after timeout. Additionally
// there is the protection from multiple scheduling.
#define CHECK_ON_TIMEOUT(id, timeout, checkFunction)                                                               \
  if (id == DrapeNotifier::kInvalidId)                                                                             \
  {                                                                                                                \
    id = m_notifier->Notify(ThreadsCommutator::RenderThread, std::chrono::seconds(static_cast<uint32_t>(timeout)), \
                            false /* repeating */, [this](uint64_t notifyId)                                       \
    {                                                                                                              \
      if (notifyId != id)                                                                                          \
        return;                                                                                                    \
      checkFunction();                                                                                             \
      id = DrapeNotifier::kInvalidId;                                                                              \
    });                                                                                                            \
  }

void MyPositionController::CheckNotFollowRouting()
{
  if (!m_blockRoutingNotFollowTimer && IsInRouting() && m_positionStatus == PositionStatus::Available &&
      m_tracking == CameraTracking::Free)
  {
    CHECK_ON_TIMEOUT(m_routingNotFollowNotifyId, kMaxNotFollowRoutingTimeSec, CheckNotFollowRouting);
    if (m_routingNotFollowTimer.ElapsedSeconds() >= kMaxNotFollowRoutingTimeSec)
    {
      StartFollowing(GetPreferredRoutingRotation());
      UpdateViewport(kDoNotChangeZoom);
    }
  }
}

void MyPositionController::CheckBlockAutoZoom()
{
  if (m_needBlockAutoZoom)
  {
    CHECK_ON_TIMEOUT(m_blockAutoZoomNotifyId, kMaxBlockAutoZoomTimeSec, CheckBlockAutoZoom);
    if (m_blockAutoZoomTimer.ElapsedSeconds() >= kMaxBlockAutoZoomTimeSec)
    {
      m_needBlockAutoZoom = false;
      m_isDirtyAutoZoom = true;
    }
  }
}

void MyPositionController::CheckUpdateLocation()
{
  if (!m_positionIsObsolete)
  {
    CHECK_ON_TIMEOUT(m_updateLocationNotifyId, kMaxUpdateLocationInvervalSec, CheckUpdateLocation);
    if (m_updateLocationTimer.ElapsedSeconds() >= kMaxUpdateLocationInvervalSec)
    {
      m_positionIsObsolete = true;
      m_autoScale2d = m_autoScale3d = kUnknownAutoZoom;
    }
  }
}

#undef CHECK_ON_TIMEOUT
}  // namespace df
