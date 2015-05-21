#include "my_position_controller.hpp"

#include "indexer/mercator.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace df
{

namespace
{

double const GPS_BEARING_LIFETIME_S = 5.0;
double const MIN_SPEED_THRESHOLD_MPS = 1.0;

uint16_t SetModeBit(uint16_t mode, uint16_t bit)
{
  return mode | bit;
}

//uint16_t ResetModeBit(uint16_t mode, uint16_t bit)
//{
//  return mode & (~bit);
//}

location::EMyPositionMode ResetAllModeBits(uint16_t mode)
{
  return (location::EMyPositionMode)(mode & 0xF);
}

uint16_t ChangeMode(uint16_t mode, location::EMyPositionMode newMode)
{
  return (mode & 0xF0) | newMode;
}

bool TestModeBit(uint16_t mode, uint16_t bit)
{
  return (mode & bit) != 0;
}

} // namespace

MyPositionController::MyPositionController(location::EMyPositionMode initMode)
  : m_modeInfo(location::MODE_PENDING_POSITION)
  , m_afterPendingMode(location::MODE_FOLLOW)
  , m_errorRadius(0.0)
  , m_position(m2::PointD::Zero())
  , m_drawDirection(0.0)
  , m_lastGPSBearing(false)
  , m_isVisible(false)
{
  if (initMode > location::MODE_UNKNOWN_POSITION)
    m_afterPendingMode = initMode;
  else
    m_modeInfo = location::MODE_UNKNOWN_POSITION;
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
  return GetMode() >= location::MODE_FOLLOW;
}

bool MyPositionController::IsModeHasPosition() const
{
  return GetMode() >= location::MODE_NOT_FOLLOW;
}

void MyPositionController::SetRenderShape(drape_ptr<MyPosition> && shape)
{
  m_shape = move(shape);
}

void MyPositionController::SetFixedZoom()
{
  SetModeInfo(SetModeBit(m_modeInfo, FixedZoomBit));
}

void MyPositionController::NextMode()
{
  string const kAlohalyticsClickEvent = "$onClick";
  location::EMyPositionMode currentMode = GetMode();
  location::EMyPositionMode newMode = currentMode;

  if (!IsInRouting())
  {
    switch (currentMode)
    {
    case location::MODE_UNKNOWN_POSITION:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@UnknownPosition");
      newMode = location::MODE_PENDING_POSITION;
      break;
    case location::MODE_PENDING_POSITION:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@PendingPosition");
      newMode = location::MODE_UNKNOWN_POSITION;
      m_afterPendingMode = location::MODE_FOLLOW;
      break;
    case location::MODE_NOT_FOLLOW:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@NotFollow");
      newMode = location::MODE_FOLLOW;
      break;
    case location::MODE_FOLLOW:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@Follow");
      if (IsRotationActive())
        newMode = location::MODE_ROTATE_AND_FOLLOW;
      else
      {
        newMode = location::MODE_UNKNOWN_POSITION;
        m_afterPendingMode = location::MODE_FOLLOW;
      }
      break;
    case location::MODE_ROTATE_AND_FOLLOW:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@RotateAndFollow");
      newMode = location::MODE_UNKNOWN_POSITION;
      m_afterPendingMode = location::MODE_FOLLOW;
      break;
    }
  }
  else
  {
    newMode = IsRotationActive() ? location::MODE_ROTATE_AND_FOLLOW : location::MODE_FOLLOW;
  }

  SetModeInfo(ChangeMode(m_modeInfo, newMode));
}

void MyPositionController::TurnOff()
{
  StopLocationFollow();
  SetModeInfo(location::MODE_UNKNOWN_POSITION);
  SetIsVisible(false);
}

void MyPositionController::Invalidate()
{
  location::EMyPositionMode currentMode = GetMode();
  if (currentMode > location::MODE_PENDING_POSITION)
  {
    SetModeInfo(ChangeMode(m_modeInfo, location::MODE_UNKNOWN_POSITION));
    SetModeInfo(ChangeMode(m_modeInfo, location::MODE_PENDING_POSITION));
    m_afterPendingMode = currentMode;
    SetIsVisible(true);
  }
  else if (currentMode == location::MODE_UNKNOWN_POSITION)
  {
    m_afterPendingMode = location::MODE_FOLLOW;
    SetIsVisible(false);
  }
}

void MyPositionController::OnLocationUpdate(location::GpsInfo const & info, bool isNavigable)
{
  Assign(info, isNavigable);

  SetIsVisible(true);

  if (GetMode() == location::MODE_PENDING_POSITION)
  {
    SetModeInfo(ChangeMode(m_modeInfo, m_afterPendingMode));
    m_afterPendingMode = location::MODE_FOLLOW;
  }
  else
  {
    AnimateFollow();
  }
}

void MyPositionController::OnCompassUpdate(location::CompassInfo const & info)
{
  if (Assign(info))
    AnimateFollow();
}

void MyPositionController::SetModeListener(location::TMyPositionModeChanged const & fn)
{
  m_modeChangeCallback = fn;
  CallModeListener(m_modeInfo);
}

void MyPositionController::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                                  dp::UniformValuesStorage const  & commonUniforms)
{
  if (m_shape != nullptr && IsVisible() && GetMode() > location::MODE_PENDING_POSITION)
  {
    m_shape->SetPosition(m_position);
    m_shape->SetAzimuth(m_drawDirection);
    m_shape->SetIsValidAzimuth(IsRotationActive());
    m_shape->SetAccuracy(m_errorRadius);
    m_shape->Render(screen, mng, commonUniforms);
  }
}

void MyPositionController::AnimateStateTransition(location::EMyPositionMode oldMode, location::EMyPositionMode newMode)
{
  //TODO UVR (rakhuba) restore viewport animation logic
}

void MyPositionController::AnimateFollow()
{
  //TODO UVR (rakhuba) restore viewport animation logic
}

void MyPositionController::Assign(location::GpsInfo const & info, bool isNavigable)
{
  m2::RectD rect = MercatorBounds::MetresToXY(info.m_longitude,
                                              info.m_latitude,
                                              info.m_horizontalAccuracy);
  m_position = rect.Center();
  m_errorRadius = rect.SizeX() / 2;

  bool const hasBearing = info.HasBearing();
  if ((isNavigable && hasBearing) ||
      (!isNavigable && hasBearing && info.HasSpeed() && info.m_speed > MIN_SPEED_THRESHOLD_MPS))
  {
    SetDirection(my::DegToRad(info.m_bearing));
    m_lastGPSBearing.Reset();
  }
}

bool MyPositionController::Assign(location::CompassInfo const & info)
{
  if ((IsInRouting() && GetMode() >= location::MODE_FOLLOW) ||
      (m_lastGPSBearing.ElapsedSeconds() < GPS_BEARING_LIFETIME_S))
  {
    return false;
  }

  SetDirection(info.m_bearing);
  return true;
}

void MyPositionController::SetDirection(double bearing)
{
  m_drawDirection = bearing;
  SetModeInfo(SetModeBit(m_modeInfo, KnownDirectionBit));
}

void MyPositionController::SetModeInfo(uint16_t modeInfo, bool force)
{
  location::EMyPositionMode const newMode = ResetAllModeBits(modeInfo);
  location::EMyPositionMode const oldMode = GetMode();
  m_modeInfo = modeInfo;
  if (newMode != oldMode || force)
  {
    AnimateStateTransition(oldMode, newMode);
    CallModeListener(newMode);
  }
}

location::EMyPositionMode MyPositionController::GetMode() const
{
  return ResetAllModeBits(m_modeInfo);
}

void MyPositionController::CallModeListener(uint16_t mode)
{
  if (m_modeChangeCallback != nullptr)
    m_modeChangeCallback(ResetAllModeBits(mode));
}

bool MyPositionController::IsInRouting() const
{
  return TestModeBit(m_modeInfo, RoutingSessionBit);
}

bool MyPositionController::IsRotationActive() const
{
  return TestModeBit(m_modeInfo, KnownDirectionBit);
}

void MyPositionController::StopLocationFollow()
{
  location::EMyPositionMode currentMode = GetMode();
  if (currentMode > location::MODE_NOT_FOLLOW)
  {
    StopAllAnimations();
    SetModeInfo(ChangeMode(m_modeInfo, location::MODE_NOT_FOLLOW));
  }
  else if (currentMode == location::MODE_PENDING_POSITION)
  {
    StopAllAnimations();
    m_afterPendingMode = location::MODE_NOT_FOLLOW;
  }
}

void MyPositionController::StopCompassFollow()
{
  if (GetMode() != location::MODE_ROTATE_AND_FOLLOW)
    return;

  StopAllAnimations();
  SetModeInfo(ChangeMode(m_modeInfo, location::MODE_FOLLOW));
}

void MyPositionController::StopAllAnimations()
{
  // TODO
}

}
