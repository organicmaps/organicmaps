#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/animation_utils.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/animation/arrow_animation.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include "std/vector.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace df
{

namespace
{

int const kPositionOffsetY = 104;
int const kPositionOffsetYIn3D = 104;
double const kGpsBearingLifetimeSec = 5.0;
double const kMinSpeedThresholdMps = 1.0;

double const kMaxPendingLocationTimeSec = 60.0;
double const kMaxTimeInBackgroundSec = 60.0 * 60;
double const kMaxNotFollowRoutingTimeSec = 20.0;
double const kMaxUpdateLocationInvervalSec = 30.0;
double const kMaxBlockAutoZoomTimeSec = 10.0;

int const kZoomThreshold = 10;
int const kMaxScaleZoomLevel = 16;
int const kDefaultAutoZoom = 16;
double const kUnknownAutoZoom = -1.0;

string LocationModeStatisticsName(location::EMyPositionMode mode)
{
  switch (mode)
  {
  case location::PendingPosition:
    return "@PendingPosition";
  case location::NotFollowNoPosition:
    return "@NotFollowNoPosition";
  case location::NotFollow:
    return "@NotFollow";
  case location::Follow:
    return "@Follow";
  case location::FollowAndRotate:
    return "@FollowAndRotate";
  }
  return "@UnknownMode";
}

int GetZoomLevel(ScreenBase const & screen)
{
  return df::GetZoomLevel(screen.GetScale());
}

int GetZoomLevel(ScreenBase const & screen, m2::PointD const & position, double errorRadius)
{
  ScreenBase s = screen;
  m2::PointD const size(errorRadius, errorRadius);
  s.SetFromRect(m2::AnyRectD(position, screen.GetAngle(), m2::RectD(position - size, position + size)));
  return GetZoomLevel(s);
}

// Calculate zoom value in meters per pixel
double CalculateZoomBySpeed(double speed, bool isPerspectiveAllowed)
{
  using TSpeedScale = pair<double, double>;
  static vector<TSpeedScale> const scales3d = {
    make_pair(20.0, 0.25),
    make_pair(40.0, 0.75),
    make_pair(60.0, 1.5),
    make_pair(75.0, 2.5),
    make_pair(85.0, 3.75),
    make_pair(95.0, 6.0),
  };

  static vector<TSpeedScale> const scales2d = {
    make_pair(20.0, 0.7),
    make_pair(40.0, 1.25),
    make_pair(60.0, 2.25),
    make_pair(75.0, 3.0),
    make_pair(85.0, 3.75),
    make_pair(95.0, 6.0),
  };

  vector<TSpeedScale> const & scales = isPerspectiveAllowed ? scales3d : scales2d;

  double const kDefaultSpeed = 80.0;
  if (speed < 0.0)
    speed = kDefaultSpeed;
  else
    speed *= 3.6; // convert speed from m/s to km/h

  size_t i = 0;
  for (size_t sz = scales.size(); i < sz; ++i)
    if (scales[i].first >= speed)
      break;

  double const vs = df::VisualParams::Instance().GetVisualScale();

  if (i == 0)
    return scales.front().second / vs;
  if (i == scales.size())
    return scales.back().second / vs;

  double const minSpeed = scales[i - 1].first;
  double const maxSpeed = scales[i].first;
  double const k = (speed - minSpeed) / (maxSpeed - minSpeed);

  double const minScale = scales[i - 1].second;
  double const maxScale = scales[i].second;
  double const zoom = minScale + k * (maxScale - minScale);

  return zoom / vs;
}

} // namespace

MyPositionController::MyPositionController(Params && params)
  : m_mode(location::PendingPosition)
  , m_desiredInitMode(params.m_initMode)
  , m_modeChangeCallback(move(params.m_myPositionModeCallback))
  , m_hints(params.m_hints)
  , m_isInRouting(params.m_isRoutingActive)
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
  , m_autoScale2d(GetScale(kDefaultAutoZoom))
  , m_autoScale3d(m_autoScale2d)
  , m_lastGPSBearing(false)
  , m_lastLocationTimestamp(0.0)
  , m_positionYOffset(kPositionOffsetY)
  , m_isVisible(false)
  , m_isDirtyViewport(false)
  , m_isDirtyAutoZoom(false)
  , m_isPendingAnimation(false)
  , m_isPositionAssigned(false)
  , m_isDirectionAssigned(false)
  , m_isCompassAvailable(false)
  , m_positionIsObsolete(false)
  , m_needBlockAutoZoom(false)
  , m_notFollowAfterPending(false)
{
  if (m_hints.m_isFirstLaunch)
  {
    m_mode = location::NotFollowNoPosition;
    m_desiredInitMode = location::NotFollowNoPosition;
  }
  else if (m_hints.m_isLaunchByDeepLink)
  {
    m_desiredInitMode = location::NotFollow;
  }
  else if (params.m_timeInBackground >= kMaxTimeInBackgroundSec)
  {
    m_desiredInitMode = location::Follow;
  }

  if (m_modeChangeCallback != nullptr)
    m_modeChangeCallback(m_mode, m_isInRouting);
}

MyPositionController::~MyPositionController()
{
}

void MyPositionController::UpdatePosition()
{
  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::OnUpdateScreen(ScreenBase const & screen)
{
  m_pixelRect = screen.PixelRectIn3d();
  m_positionYOffset = screen.isPerspective() ? kPositionOffsetYIn3D : kPositionOffsetY;
  if (m_visiblePixelRect.IsEmptyInterior())
    m_visiblePixelRect = m_pixelRect;
}

void MyPositionController::SetVisibleViewport(const m2::RectD &rect)
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
  return m_mode == location::Follow || m_mode == location::FollowAndRotate;
}

bool MyPositionController::IsModeHasPosition() const
{
  return m_mode != location::PendingPosition && m_mode != location::NotFollowNoPosition;
}

void MyPositionController::DragStarted()
{
  m_needBlockAnimation = true;
}

void MyPositionController::DragEnded(m2::PointD const & distance)
{
  float const kBindingDistance = 0.1;
  m_needBlockAnimation = false;
  if (distance.Length() > kBindingDistance * min(m_pixelRect.SizeX(), m_pixelRect.SizeY()))
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
  if (m_mode == location::FollowAndRotate)
    m_wasRotationInScaling = true;
}

void MyPositionController::ResetRoutingNotFollowTimer()
{
  if (m_isInRouting)
    m_routingNotFollowTimer.Reset();
}

void MyPositionController::ResetBlockAutoZoomTimer()
{
  if (m_isInRouting && m_enableAutoZoomInRouting)
  {
    m_needBlockAutoZoom = true;
    m_blockAutoZoomTimer.Reset();
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

void MyPositionController::SetRenderShape(drape_ptr<MyPosition> && shape)
{
  m_shape = move(shape);
}

void MyPositionController::ResetRenderShape()
{
  m_shape.reset();
}

void MyPositionController::NextMode(ScreenBase const & screen)
{
  string const kAlohalyticsClickEvent = "$onClick";

  // Skip switching to next mode while we are waiting for position.
  if (IsWaitingForLocation())
  {
    alohalytics::LogEvent(kAlohalyticsClickEvent,
                          LocationModeStatisticsName(location::PendingPosition));
    return;
  }

  alohalytics::LogEvent(kAlohalyticsClickEvent, LocationModeStatisticsName(m_mode));

  // Start looking for location.
  if (m_mode == location::NotFollowNoPosition)
  {
    m_pendingTimer.Reset();
    ChangeMode(location::PendingPosition);
    return;
  }

  // Calculate preferred zoom level.
  int const currentZoom = GetZoomLevel(screen);
  int preferredZoomLevel = kDoNotChangeZoom;
  if (currentZoom < kZoomThreshold)
    preferredZoomLevel = min(GetZoomLevel(screen, m_position, m_errorRadius), kMaxScaleZoomLevel);

  // In routing not-follow -> follow-and-rotate, otherwise not-follow -> follow.
  if (m_mode == location::NotFollow)
  {
    ChangeMode(m_isInRouting ? location::FollowAndRotate : location::Follow);
    UpdateViewport(preferredZoomLevel);
    return;
  }

  // From follow mode we transit to follow-and-rotate if compass is available or
  // routing is enabled.
  if (m_mode == location::Follow)
  {
    if (IsRotationAvailable() || m_isInRouting)
    {
      ChangeMode(location::FollowAndRotate);
      UpdateViewport(preferredZoomLevel);
    }
    return;
  }

  // From follow-and-rotate mode we can transit to follow mode.
  if (m_mode == location::FollowAndRotate)
  {
    if (m_isInRouting && screen.isPerspective())
      preferredZoomLevel = GetZoomLevel(ScreenBase::GetStartPerspectiveScale() * 1.1);
    ChangeMode(location::Follow);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), preferredZoomLevel);
  }
}

void MyPositionController::OnLocationUpdate(location::GpsInfo const & info, bool isNavigable,
                                            ScreenBase const & screen)
{
  m2::PointD const oldPos = GetDrawablePosition();
  double const oldAzimut = GetDrawableAzimut();

  m2::RectD const rect = MercatorBounds::MetresToXY(info.m_longitude, info.m_latitude,
                                                    info.m_horizontalAccuracy);
  // Use FromLatLon instead of rect.Center() since in case of large info.m_horizontalAccuracy
  // there is significant difference between the real location and the estimated one.
  m_position = MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude);
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

  bool const hasBearing = info.HasBearing();
  if ((isNavigable && hasBearing) ||
      (!isNavigable && hasBearing && info.HasSpeed() && info.m_speed > kMinSpeedThresholdMps))
  {
    SetDirection(my::DegToRad(info.m_bearing));
    m_lastGPSBearing.Reset();
  }

  if (m_isPositionAssigned && (!AlmostCurrentPosition(oldPos) || !AlmostCurrentAzimut(oldAzimut)))
  {
    CreateAnim(oldPos, oldAzimut, screen);
    m_isDirtyViewport = true;
  }

  if (m_notFollowAfterPending && m_mode == location::PendingPosition)
  {
    ChangeMode(location::NotFollow);
    if (m_isInRouting)
      m_routingNotFollowTimer.Reset();
    m_notFollowAfterPending = false;
  }
  else if (!m_isPositionAssigned)
  {
    ChangeMode(m_hints.m_isFirstLaunch ? location::Follow : m_desiredInitMode);
    if (!m_hints.m_isFirstLaunch || !AnimationSystem::Instance().AnimationExists(Animation::Object::MapPlane))
    {
      if (m_mode == location::Follow)
      {
        ChangeModelView(m_position, kDoNotChangeZoom);
      }
      else if (m_mode == location::FollowAndRotate)
      {
        ChangeModelView(m_position, m_drawDirection,
                        m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center(),
                        kDoNotChangeZoom);
      }
    }
  }
  else if (m_mode == location::PendingPosition || m_mode == location::NotFollowNoPosition)
  {
    if (m_isInRouting)
    {
      ChangeMode(location::FollowAndRotate);
      UpdateViewport(kMaxScaleZoomLevel);
    }
    else
    {
      ChangeMode(location::Follow);
      if (!m_hints.m_isFirstLaunch)
      {
        if (GetZoomLevel(screen, m_position, m_errorRadius) <= kMaxScaleZoomLevel)
        {
          m2::PointD const size(m_errorRadius, m_errorRadius);
          ChangeModelView(m2::RectD(m_position - size, m_position + size));
        }
        else
        {
          ChangeModelView(m_position, kMaxScaleZoomLevel);
        }
      }
      else
      {
        if (!AnimationSystem::Instance().AnimationExists(Animation::Object::MapPlane))
          ChangeModelView(m_position, kDoNotChangeZoom);
      }
    }
  }

  m_isPositionAssigned = true;
  m_positionIsObsolete = false;
  SetIsVisible(true);

  if (m_listener != nullptr)
    m_listener->PositionChanged(Position(), IsModeHasPosition());

  double const kEps = 1e-5;
  if (fabs(m_lastLocationTimestamp - info.m_timestamp) > kEps)
  {
    m_lastLocationTimestamp = info.m_timestamp;
    m_updateLocationTimer.Reset();
  }
}

void MyPositionController::LoseLocation()
{
  if (m_mode != location::NotFollowNoPosition)
  {
    ChangeMode(location::NotFollowNoPosition);
    SetIsVisible(false);
    if (m_listener != nullptr)
      m_listener->PositionChanged(Position(), false /* hasPosition */);
  }
}

void MyPositionController::OnCompassUpdate(location::CompassInfo const & info, ScreenBase const & screen)
{
  double const oldAzimut = GetDrawableAzimut();
  m_isCompassAvailable = true;

  if ((IsInRouting() && m_mode == location::FollowAndRotate) ||
      m_lastGPSBearing.ElapsedSeconds() < kGpsBearingLifetimeSec)
    return;

  SetDirection(info.m_bearing);

  if (m_isPositionAssigned && !AlmostCurrentAzimut(oldAzimut) && m_mode == location::FollowAndRotate)
  {
    CreateAnim(GetDrawablePosition(), oldAzimut, screen);
    m_isDirtyViewport = true;
  }
}

bool MyPositionController::IsInStateWithPosition() const
{
  return m_mode == location::NotFollow || m_mode == location::Follow ||
         m_mode == location::FollowAndRotate;
}

bool MyPositionController::UpdateViewportWithAutoZoom()
{
  double autoScale = m_enablePerspectiveInRouting ? m_autoScale3d : m_autoScale2d;
  if (autoScale > 0.0 && m_mode == location::FollowAndRotate &&
      m_isInRouting && m_enableAutoZoomInRouting && !m_needBlockAutoZoom)
  {
    ChangeModelView(autoScale, m_position, m_drawDirection, GetRoutingRotationPixelCenter());
    return true;
  }
  return false;
}

void MyPositionController::Render(ScreenBase const & screen, int zoomLevel,
                                  ref_ptr<dp::GpuProgramManager> mng,
                                  dp::UniformValuesStorage const & commonUniforms)
{
  if (IsWaitingForLocation())
  {
    if (m_pendingTimer.ElapsedSeconds() >= kMaxPendingLocationTimeSec)
      ChangeMode(location::NotFollowNoPosition);
  }

  if (IsInRouting() && m_mode == location::NotFollow &&
      m_routingNotFollowTimer.ElapsedSeconds() >= kMaxNotFollowRoutingTimeSec)
  {
    ChangeMode(location::FollowAndRotate);
    UpdateViewport(kDoNotChangeZoom);
  }

  if (m_shape != nullptr && IsVisible() && IsModeHasPosition())
  {
    if (m_needBlockAutoZoom &&
        m_blockAutoZoomTimer.ElapsedSeconds() >= kMaxBlockAutoZoomTimeSec)
    {
      m_needBlockAutoZoom = false;
      m_isDirtyAutoZoom = true;
    }

    if (!m_positionIsObsolete &&
        m_updateLocationTimer.ElapsedSeconds() >= kMaxUpdateLocationInvervalSec)
    {
      m_positionIsObsolete = true;
      m_autoScale2d = m_autoScale3d = kUnknownAutoZoom;
    }

    if ((m_isDirtyViewport || m_isDirtyAutoZoom) && !m_needBlockAnimation)
    {
      if (!UpdateViewportWithAutoZoom() && m_isDirtyViewport)
        UpdateViewport(kDoNotChangeZoom);
      m_isDirtyViewport = false;
      m_isDirtyAutoZoom = false;
    }

    if (!IsModeChangeViewport())
      m_isPendingAnimation = false;

    m_shape->SetPositionObsolete(m_positionIsObsolete);
    m_shape->SetPosition(GetDrawablePosition());
    m_shape->SetAzimuth(GetDrawableAzimut());
    m_shape->SetIsValidAzimuth(IsRotationAvailable());
    m_shape->SetAccuracy(m_errorRadius);
    m_shape->SetRoutingMode(IsInRouting());

    m_shape->RenderAccuracy(screen, zoomLevel, mng, commonUniforms);
    m_shape->RenderMyPosition(screen, zoomLevel, mng, commonUniforms);
  }
}

bool MyPositionController::IsRouteFollowingActive() const
{
  return IsInRouting() && m_mode == location::FollowAndRotate;
}

bool MyPositionController::AlmostCurrentPosition(m2::PointD const & pos) const
{
  double const kPositionEqualityDelta = 1e-5;
  return pos.EqualDxDy(m_position, kPositionEqualityDelta);
}

bool MyPositionController::AlmostCurrentAzimut(double azimut) const
{
  double const kDirectionEqualityDelta = 1e-5;
  return my::AlmostEqualAbs(azimut, m_drawDirection, kDirectionEqualityDelta);
}

void MyPositionController::SetDirection(double bearing)
{
  m_drawDirection = bearing;
  m_isDirectionAssigned = true;
}

void MyPositionController::ChangeMode(location::EMyPositionMode newMode)
{
  if (m_isInRouting && (m_mode != newMode) && (newMode == location::FollowAndRotate))
    ResetBlockAutoZoomTimer();

  m_mode = newMode;
  if (m_modeChangeCallback != nullptr)
    m_modeChangeCallback(m_mode, m_isInRouting);
}

bool MyPositionController::IsWaitingForLocation() const
{
  if (m_mode == location::NotFollowNoPosition)
    return false;
  
  if (!m_isPositionAssigned)
    return true;

  return m_mode == location::PendingPosition;
}

bool MyPositionController::IsWaitingForTimers() const
{
  return IsWaitingForLocation() || (IsInRouting() && m_mode == location::NotFollow);
}

void MyPositionController::StopLocationFollow()
{
  if (m_mode == location::Follow || m_mode == location::FollowAndRotate)
    ChangeMode(location::NotFollow);
  m_desiredInitMode = location::NotFollow;

  if (m_mode == location::PendingPosition)
    m_notFollowAfterPending = true;
  
  if (m_isInRouting)
    m_routingNotFollowTimer.Reset();
}

void MyPositionController::SetTimeInBackground(double time)
{
  if (time >= kMaxTimeInBackgroundSec && m_mode == location::NotFollow)
  {
    ChangeMode(m_isInRouting ? location::FollowAndRotate : location::Follow);
    UpdateViewport(kDoNotChangeZoom);
  }
}

void MyPositionController::OnCompassTapped()
{
  alohalytics::LogEvent("$compassClicked", {{"mode", LocationModeStatisticsName(m_mode)},
                                            {"routing", strings::to_string(IsInRouting())}});
  if (m_mode == location::FollowAndRotate)
  {
    ChangeMode(location::Follow);
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

void MyPositionController::ChangeModelView(m2::PointD const & userPos, double azimuth,
                                           m2::PointD const & pxZero, int zoomLevel,
                                           Animation::TAction const & onFinishAction)
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
  
  if (m_mode == location::Follow)
    ChangeModelView(m_position, zoomLevel);
  else if (m_mode == location::FollowAndRotate)
    ChangeModelView(m_position, m_drawDirection,
                    m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center(), zoomLevel);
}

m2::PointD MyPositionController::GetRotationPixelCenter() const
{
  if (m_mode == location::Follow)
    return m_visiblePixelRect.Center();
  
  if (m_mode == location::FollowAndRotate)
    return m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center();

  return m2::PointD::Zero();
}

m2::PointD MyPositionController::GetRoutingRotationPixelCenter() const
{
  return m2::PointD(m_visiblePixelRect.Center().x,
                    m_visiblePixelRect.maxY() - m_positionYOffset * VisualParams::Instance().GetVisualScale());
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
  if (df::IsAnimationAllowed(max(moveDuration, rotateDuration), screen))
  {
    if (IsModeChangeViewport())
    {
      m_animCreator = [this, moveDuration](ref_ptr<Animation> syncAnim) -> drape_ptr<Animation>
      {
        drape_ptr<Animation> anim = make_unique_dp<ArrowAnimation>(GetDrawablePosition(), m_position,
                                                   syncAnim == nullptr ? moveDuration : syncAnim->GetDuration(),
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
      AnimationSystem::Instance().CombineAnimation(make_unique_dp<ArrowAnimation>(oldPos, m_position, moveDuration,
                                                                                  oldAzimut, m_drawDirection));
    }
  }
}

void MyPositionController::EnablePerspectiveInRouting(bool enablePerspective)
{
  m_enablePerspectiveInRouting = enablePerspective;
}

void MyPositionController::EnableAutoZoomInRouting(bool enableAutoZoom)
{
  if (m_isInRouting)
  {
    m_enableAutoZoomInRouting = enableAutoZoom;
    ResetBlockAutoZoomTimer();
  }
}

void MyPositionController::ActivateRouting(int zoomLevel, bool enableAutoZoom)
{
  if (!m_isInRouting)
  {
    m_isInRouting = true;
    m_routingNotFollowTimer.Reset();
    m_enableAutoZoomInRouting = enableAutoZoom;

    ChangeMode(location::FollowAndRotate);
    ChangeModelView(m_position, m_isDirectionAssigned ? m_drawDirection : 0.0,
                    GetRoutingRotationPixelCenter(), zoomLevel,
                    [this](ref_ptr<Animation> anim)
                    {
                      UpdateViewport(kDoNotChangeZoom);
                    });

  }
}

void MyPositionController::DeactivateRouting()
{
  if (m_isInRouting)
  {
    m_isInRouting = false;

    m_isDirectionAssigned = m_isCompassAvailable && m_isDirectionAssigned;

    ChangeMode(location::Follow);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), kDoNotChangeZoom);
  }
}

}
