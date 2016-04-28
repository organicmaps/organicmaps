#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/animation_utils.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace df
{

namespace
{

int const kPositionOffsetY = 75;
int const kPositionOffsetYIn3D = 80;
double const kGpsBearingLifetimeSec = 5.0;
double const kMinSpeedThresholdMps = 1.0;

double const kMaxPendingLocationTimeSec = 60.0;
double const kMaxTimeInBackgroundSec = 60.0 * 60;
double const kMaxNotFollowRoutingTimeSec = 10.0;
double const kMaxUpdateLocationInvervalSec = 30.0;

int const kDoNotChangeZoom = -1;

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

} // namespace

class MyPositionController::MyPositionAnim : public BaseInterpolator
{
  using TBase = BaseInterpolator;
public:
  MyPositionAnim(m2::PointD const & startPt, m2::PointD const & endPt, double moveDuration,
                 double startAzimut, double endAzimut, double rotationDuration)
    : TBase(max(moveDuration, rotationDuration))
    , m_startPt(startPt)
    , m_endPt(endPt)
    , m_moveDuration(moveDuration)
    , m_angleInterpolator(startAzimut, endAzimut)
    , m_rotateDuration(rotationDuration)
  {
  }

  m2::PointD GetCurrentPosition() const
  {
    return InterpolatePoint(m_startPt, m_endPt,
                            my::clamp(GetElapsedTime() / m_moveDuration, 0.0, 1.0));
  }

  bool IsMovingActive() const { return m_moveDuration > 0.0; }

  double GetCurrentAzimut() const
  {
    return m_angleInterpolator.Interpolate(my::clamp(GetElapsedTime() / m_rotateDuration, 0.0, 1.0));
  }

  bool IsRotatingActive() const { return m_rotateDuration > 0.0; }

private:
  m2::PointD m_startPt;
  m2::PointD m_endPt;
  double m_moveDuration;

  InerpolateAngle m_angleInterpolator;
  double m_rotateDuration;
};

MyPositionController::MyPositionController(location::EMyPositionMode initMode,
                                           double timeInBackground, bool isFirstLaunch)
  : m_mode(initMode)
  , m_isFirstLaunch(isFirstLaunch)
  , m_isInRouting(false)
  , m_needBlockAnimation(false)
  , m_wasRotationInScaling(false)
  , m_errorRadius(0.0)
  , m_position(m2::PointD::Zero())
  , m_drawDirection(0.0)
  , m_oldPosition(m2::PointD::Zero())
  , m_oldDrawDirection(0.0)
  , m_lastGPSBearing(false)
  , m_lastLocationTimestamp(0.0)
  , m_positionYOffset(kPositionOffsetY)
  , m_isVisible(false)
  , m_isDirtyViewport(false)
  , m_isPendingAnimation(false)
  , m_isPositionAssigned(false)
  , m_isDirectionAssigned(false)
{
  if (isFirstLaunch)
    m_mode = location::NotFollowNoPosition;
  else if (m_mode == location::NotFollowNoPosition || timeInBackground >= kMaxTimeInBackgroundSec)
    m_mode = location::Follow;
}

MyPositionController::~MyPositionController()
{
  m_anim.reset();
}

void MyPositionController::OnNewPixelRect()
{
  UpdateViewport();
}

void MyPositionController::UpdatePixelPosition(ScreenBase const & screen)
{
  m_pixelRect = screen.isPerspective() ? screen.PixelRectIn3d() : screen.PixelRect();
  m_positionYOffset = screen.isPerspective() ? kPositionOffsetYIn3D : kPositionOffsetY;
  m_centerPixelPositionRouting = screen.P3dtoP(GetRoutingRotationPixelCenter());
  m_centerPixelPosition = screen.P3dtoP(m_pixelRect.Center());
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

  UpdateViewport();
}

void MyPositionController::ScaleStarted()
{
  m_needBlockAnimation = true;
}

void MyPositionController::ScaleEnded()
{
  m_needBlockAnimation = false;
  if (m_wasRotationInScaling)
  {
    m_wasRotationInScaling = false;
    StopLocationFollow();
  }

  UpdateViewport();
}

void MyPositionController::Rotated()
{
  if (m_mode == location::FollowAndRotate)
    m_wasRotationInScaling = true;
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

void MyPositionController::NextMode()
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

  // In routing not-follow -> follow-and-rotate, otherwise not-follow -> follow.
  if (m_mode == location::NotFollow)
  {
    ChangeMode(m_isInRouting ? location::FollowAndRotate : location::Follow);
    UpdateViewport();
    return;
  }

  // From follow mode we transit to follow-and-rotate if compass is available or
  // routing is enabled.
  if (m_mode == location::Follow)
  {
    if (IsRotationAvailable() || m_isInRouting)
    {
      ChangeMode(location::FollowAndRotate);
      UpdateViewport();
    }
    return;
  }

  // From follow-and-rotate mode we can transit to follow mode if routing is disabled.
  if (m_mode == location::FollowAndRotate)
  {
    if (!m_isInRouting)
    {
      ChangeMode(location::Follow);
      UpdateViewport();
    }
  }
}

void MyPositionController::OnLocationUpdate(location::GpsInfo const & info, bool isNavigable,
                                            ScreenBase const & screen)
{
  m2::PointD const oldPos = GetDrawablePosition();
  double const oldAzimut = GetDrawableAzimut();

  m2::RectD const rect = MercatorBounds::MetresToXY(info.m_longitude, info.m_latitude,
                                                    info.m_horizontalAccuracy);
  m_position = rect.Center();
  m_errorRadius = rect.SizeX() * 0.5;

  bool const hasBearing = info.HasBearing();
  if ((isNavigable && hasBearing) ||
      (!isNavigable && hasBearing && info.HasSpeed() && info.m_speed > kMinSpeedThresholdMps))
  {
    SetDirection(my::DegToRad(info.m_bearing));
    m_lastGPSBearing.Reset();
  }

  if (m_listener)
    m_listener->PositionChanged(Position());

  if (m_isPositionAssigned && (!AlmostCurrentPosition(oldPos) || !AlmostCurrentAzimut(oldAzimut)))
  {
    CreateAnim(oldPos, oldAzimut, screen);
    m_isDirtyViewport = true;
  }

  if (m_mode == location::PendingPosition || m_mode == location::NotFollowNoPosition)
  {
    ChangeMode(location::Follow);
    if (!m_isFirstLaunch)
    {
      m2::PointD const size(m_errorRadius, m_errorRadius);
      ChangeModelView(m2::RectD(m_position - size, m_position + size));
    }
    else
    {
      if (!AnimationSystem::Instance().AnimationExists(Animation::MapPlane))
        ChangeModelView(m_position, kDoNotChangeZoom);
    }
  }
  else if (!m_isPositionAssigned)
  {
    ChangeMode(m_mode);
    if (m_mode == location::Follow)
      ChangeModelView(m_position, kDoNotChangeZoom);
    else if (m_mode == location::FollowAndRotate)
      ChangeModelView(m_position, m_drawDirection,
                      m_isInRouting ? m_centerPixelPositionRouting : m_centerPixelPosition, kDoNotChangeZoom);
  }

  m_isPositionAssigned = true;
  SetIsVisible(true);

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
  }
}

void MyPositionController::OnCompassUpdate(location::CompassInfo const & info, ScreenBase const & screen)
{
  double const oldAzimut = GetDrawableAzimut();

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

void MyPositionController::SetModeListener(location::TMyPositionModeChanged const & fn)
{
  m_modeChangeCallback = fn;

  location::EMyPositionMode mode = m_mode;
  if (m_isFirstLaunch)
    mode = location::NotFollowNoPosition;
  else if (!m_isPositionAssigned)
    mode = location::PendingPosition;

  if (m_modeChangeCallback != nullptr)
    m_modeChangeCallback(mode, m_isInRouting);
}

void MyPositionController::Render(uint32_t renderMode, ScreenBase const & screen,
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
    UpdateViewport();
  }

  if (m_shape != nullptr && IsVisible() && IsModeHasPosition())
  {
    if (m_isDirtyViewport && !m_needBlockAnimation)
    {
      UpdateViewport();
      m_isDirtyViewport = false;
    }

    if (!IsModeChangeViewport())
      m_isPendingAnimation = false;

    m_shape->SetPosition(GetDrawablePosition());
    m_shape->SetAzimuth(GetDrawableAzimut());
    m_shape->SetIsValidAzimuth(IsRotationAvailable());
    m_shape->SetAccuracy(m_errorRadius);
    m_shape->SetRoutingMode(IsInRouting());

    double const updateInterval = m_updateLocationTimer.ElapsedSeconds();
    m_shape->SetPositionObsolete(updateInterval >= kMaxUpdateLocationInvervalSec);

    if ((renderMode & RenderAccuracy) != 0)
      m_shape->RenderAccuracy(screen, mng, commonUniforms);

    if ((renderMode & RenderMyPosition) != 0)
      m_shape->RenderMyPosition(screen, mng, commonUniforms);
  }

  CheckAnimFinished();
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
  
  if (m_isInRouting)
    m_routingNotFollowTimer.Reset();
}

void MyPositionController::SetTimeInBackground(double time)
{
  if (time >= kMaxTimeInBackgroundSec && m_mode == location::NotFollow)
  {
    ChangeMode(location::Follow);
    UpdateViewport();
  }
}

void MyPositionController::OnCompassTapped()
{
  alohalytics::LogEvent("$compassClicked", {{"mode", LocationModeStatisticsName(m_mode)},
                                            {"routing", strings::to_string(IsInRouting())}});
  ChangeModelView(0.0);
  if (m_mode == location::FollowAndRotate)
  {
    ChangeMode(location::Follow);
    UpdateViewport();
  }
}

void MyPositionController::ChangeModelView(m2::PointD const & center, int zoomLevel)
{
  if (m_listener)
    m_listener->ChangeModelView(center, zoomLevel);
}

void MyPositionController::ChangeModelView(double azimuth)
{
  if (m_listener)
    m_listener->ChangeModelView(azimuth);
}

void MyPositionController::ChangeModelView(m2::RectD const & rect)
{
  if (m_listener)
    m_listener->ChangeModelView(rect);
}

void MyPositionController::ChangeModelView(m2::PointD const & userPos, double azimuth,
                                           m2::PointD const & pxZero, int zoomLevel)
{
  if (m_listener)
    m_listener->ChangeModelView(userPos, azimuth, pxZero, zoomLevel);
}

void MyPositionController::UpdateViewport()
{
  if (IsWaitingForLocation())
    return;
  
  if (m_mode == location::Follow)
    ChangeModelView(m_position, kDoNotChangeZoom);
  else if (m_mode == location::FollowAndRotate)
    ChangeModelView(m_position, m_drawDirection,
                    m_isInRouting ? m_centerPixelPositionRouting : m_centerPixelPosition, kDoNotChangeZoom);
}

m2::PointD MyPositionController::GetRotationPixelCenter() const
{
  if (m_mode == location::Follow)
    return m_pixelRect.Center();
  
  if (m_mode == location::FollowAndRotate)
    return m_isInRouting ? GetRoutingRotationPixelCenter() : m_pixelRect.Center();

  return m2::PointD::Zero();
}

m2::PointD MyPositionController::GetRoutingRotationPixelCenter() const
{
  return m2::PointD(m_pixelRect.Center().x,
                    m_pixelRect.maxY() - m_positionYOffset * VisualParams::Instance().GetVisualScale());
}

m2::PointD MyPositionController::GetDrawablePosition() const
{
  if (m_anim != nullptr && m_anim->IsMovingActive())
    return m_anim->GetCurrentPosition();

  if (m_isPendingAnimation)
    return m_oldPosition;

  return m_position;
}

double MyPositionController::GetDrawableAzimut() const
{
  if (m_anim != nullptr && m_anim->IsRotatingActive())
    return m_anim->GetCurrentAzimut();

  if (m_isPendingAnimation)
    return m_oldDrawDirection;

  return m_drawDirection;
}

void MyPositionController::CheckAnimFinished() const
{
  if (m_anim && m_anim->IsFinished())
    m_anim.reset();
}

void MyPositionController::AnimationStarted(ref_ptr<Animation> anim)
{
  if (m_isPendingAnimation && m_animCreator != nullptr && anim != nullptr &&
      (anim->GetType() == Animation::MapFollow ||
       anim->GetType() == Animation::MapLinear))
  {
    m_isPendingAnimation = false;
    m_animCreator();
  }
}

void MyPositionController::CreateAnim(m2::PointD const & oldPos, double oldAzimut, ScreenBase const & screen)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(oldPos, m_position, screen);
  double const rotateDuration = AngleInterpolator::GetRotateDuration(oldAzimut, m_drawDirection);
  if (df::IsAnimationAllowed(max(moveDuration, rotateDuration), screen))
  {
    if (IsModeChangeViewport())
    {
      m_animCreator = [this, oldPos, moveDuration, oldAzimut, rotateDuration]()
      {
        m_anim = make_unique_dp<MyPositionAnim>(oldPos, m_position, moveDuration, oldAzimut, m_drawDirection, rotateDuration);
      };
      m_oldPosition = oldPos;
      m_oldDrawDirection = oldAzimut;
      m_isPendingAnimation = true;
    }
    else
    {
      m_anim = make_unique_dp<MyPositionAnim>(oldPos, m_position, moveDuration, oldAzimut, m_drawDirection, rotateDuration);
    }
  }
}

void MyPositionController::ActivateRouting(int zoomLevel)
{
  if (!m_isInRouting)
  {
    m_isInRouting = true;

    if (IsRotationAvailable())
    {
      ChangeMode(location::FollowAndRotate);
      ChangeModelView(m_position, m_drawDirection, m_centerPixelPositionRouting, zoomLevel);
    }
    else
    {
      ChangeMode(location::Follow);
      ChangeModelView(m_position, zoomLevel);
    }
  }
}

void MyPositionController::DeactivateRouting()
{
  if (m_isInRouting)
  {
    m_isInRouting = false;

    ChangeMode(location::Follow);
    ChangeModelView(m_position, 0.0, m_centerPixelPosition, kDoNotChangeZoom);
  }
}

}
