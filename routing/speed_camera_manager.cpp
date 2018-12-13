#include "routing/speed_camera_manager.hpp"

#include "routing/speed_camera.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace routing
{
std::string const SpeedCameraManager::kSpeedCamModeKey = "speed_cam_mode";

SpeedCameraManager::SpeedCameraManager(turns::sound::NotificationManager & notificationManager)
  : m_notificationManager(notificationManager)
{
  Reset();

  uint32_t mode = 0;
  if (settings::Get(kSpeedCamModeKey, mode))
  {
    CHECK_LESS(mode, static_cast<int>(SpeedCameraManagerMode::MaxValue), ("Invalid speedcam mode."));
    m_mode = static_cast<SpeedCameraManagerMode>(mode);
  }
}

//static
SpeedCameraManager::Interval
SpeedCameraManager::GetIntervalByDistToCam(double distanceToCameraMeters, double speedMpS)
{
  if (distanceToCameraMeters < kInfluenceZoneMeters)
    return Interval::ImpactZone;

  double const beepDist = kBeepSignalTime * speedMpS;
  if (distanceToCameraMeters < kInfluenceZoneMeters + beepDist)
    return Interval::BeepSignalZone;

  return Interval::VoiceNotificationZone;
}

void SpeedCameraManager::OnLocationPositionChanged(location::GpsInfo const & info)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(!m_route.expired(), ());

  auto const passedDistanceMeters = m_route.lock()->GetCurrentDistanceFromBeginMeters();

  // Step 1. Find new cameras and cache them.
  FindCamerasOnRouteAndCache(passedDistanceMeters);

  double distFromCurrentPosAndClosestCam = -1.0;
  m_speedLimitExceeded = false;
  if (m_closestCamera.IsValid())
  {
    distFromCurrentPosAndClosestCam = m_closestCamera.m_distFromBeginMeters - passedDistanceMeters;
    if (distFromCurrentPosAndClosestCam < -kInfluenceZoneMeters)
      m_closestCamera.Invalidate();
    else if (!m_closestCamera.NoSpeed())
      m_speedLimitExceeded = IsSpeedHigh(distFromCurrentPosAndClosestCam, info.m_speedMpS, m_closestCamera);
  }

  // Step 2. Check cached cameras. Do it only after pass through closest camera.
  if (!m_cachedSpeedCameras.empty() && distFromCurrentPosAndClosestCam < 0)
  {
    // Do not use reference here, because ProcessCameraWarning() can
    // invalidate |closestSpeedCam|.
    auto const closestSpeedCam = m_cachedSpeedCameras.front();

    bool needUpdateClosestCamera = false;
    auto const distToCamMeters = closestSpeedCam.m_distFromBeginMeters - passedDistanceMeters;

    if (closestSpeedCam.m_distFromBeginMeters + kInfluenceZoneMeters > passedDistanceMeters)
    {
      needUpdateClosestCamera =
        NeedUpdateClosestCamera(distToCamMeters, info.m_speedMpS, closestSpeedCam);

      if (needUpdateClosestCamera)
      {
        m_closestCamera = closestSpeedCam;
        ResetNotifications();
        m_cachedSpeedCameras.pop();
      }
    }

    if (NeedChangeHighlightedCamera(distToCamMeters, needUpdateClosestCamera))
      PassCameraToUI(closestSpeedCam);
  }

  if (m_closestCamera.IsValid() &&
      SetNotificationFlags(passedDistanceMeters, info.m_speedMpS, m_closestCamera))
  {
    // If some notifications available now.
    SendNotificationStat(passedDistanceMeters, info.m_speedMpS, m_closestCamera);
  }

  // Step 3. Check UI camera (stop or not stop to highlight it).
  if (!m_currentHighlightedCamera.IsValid())
    return;

  auto const distToCameraMeters =
    m_currentHighlightedCamera.m_distFromBeginMeters - passedDistanceMeters;

  if (IsHighlightedCameraExpired(distToCameraMeters))
  {
    m_speedCamClearCallback();
    m_currentHighlightedCamera.Invalidate();
  }
}

void SpeedCameraManager::GenerateNotifications(std::vector<std::string> & notifications)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!Enable())
    return;

  if (VoiceSignalAvailable())
  {
    notifications.emplace_back(m_notificationManager.GenerateSpeedCameraText());
    m_makeVoiceSignal = false;
    ++m_voiceSignalCounter;
  }
}

bool SpeedCameraManager::ShouldPlayBeepSignal()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!Enable())
    return false;

  if (BeepSignalAvailable())
  {
    m_makeBeepSignal = false;
    ++m_beepSignalCounter;
    return true;
  }

  return false;
}

void SpeedCameraManager::ResetNotifications()
{
  m_makeVoiceSignal = false;
  m_makeBeepSignal = false;
  m_speedLimitExceeded = false;
  m_beepSignalCounter = 0;
  m_voiceSignalCounter = 0;
  m_hasEnteredTheZone = false;
}

void SpeedCameraManager::Reset()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  ResetNotifications();

  m_speedCamClearCallback();

  m_closestCamera.Invalidate();
  m_currentHighlightedCamera.Invalidate();

  m_firstNotCheckedSpeedCameraIndex = 1;
  m_cachedSpeedCameras = std::queue<SpeedCameraOnRoute>();
}

bool SpeedCameraManager::IsSpeedLimitExceeded() const
{
  if (!m_closestCamera.IsValid())
    return false;

  return m_speedLimitExceeded;
}

std::string SpeedCameraManagerModeForStat(SpeedCameraManagerMode mode)
{
  CHECK_NOT_EQUAL(mode, SpeedCameraManagerMode::MaxValue, ());

  switch(mode)
  {
  case SpeedCameraManagerMode::Always: return "1";
  case SpeedCameraManagerMode::Auto: return "0";
  case SpeedCameraManagerMode::Never: return "-1";
  case SpeedCameraManagerMode::MaxValue: return "-2";
  }

  UNREACHABLE();
}

void SpeedCameraManager::FindCamerasOnRouteAndCache(double passedDistanceMeters)
{
  CHECK(!m_route.expired(), ());

  auto const & segments = m_route.lock()->GetRouteSegments();
  size_t firstNotChecked = m_firstNotCheckedSpeedCameraIndex;
  if (firstNotChecked == segments.size())
    return;

  CHECK_LESS(firstNotChecked, segments.size(), ());

  double distToPrevSegment = segments[firstNotChecked].GetDistFromBeginningMeters();
  double distFromCurPosToLatestCheckedSegmentM = distToPrevSegment - passedDistanceMeters;

  while (firstNotChecked < segments.size() &&
         distFromCurPosToLatestCheckedSegmentM < kLookAheadDistanceMeters)
  {
    CHECK_GREATER(firstNotChecked, 0, ());

    auto const & lastSegment = segments[firstNotChecked];
    auto const & prevSegment = segments[firstNotChecked - 1];

    auto const & endPoint = lastSegment.GetJunction().GetPoint();
    auto const & startPoint = prevSegment.GetJunction().GetPoint();
    auto const direction = endPoint - startPoint;

    auto const & speedCamsVector = lastSegment.GetSpeedCams();
    double segmentLength = m_route.lock()->GetSegLenMeters(firstNotChecked);

    for (auto const & speedCam : speedCamsVector)
    {
      segmentLength *= speedCam.m_coef;
      m_cachedSpeedCameras.emplace(distToPrevSegment + segmentLength, speedCam.m_maxSpeedKmPH,
                                   startPoint + direction * speedCam.m_coef);
    }

    distToPrevSegment = lastSegment.GetDistFromBeginningMeters();
    distFromCurPosToLatestCheckedSegmentM = distToPrevSegment - passedDistanceMeters;
    ++firstNotChecked;
  }

  m_firstNotCheckedSpeedCameraIndex = firstNotChecked;
}

void SpeedCameraManager::PassCameraToUI(SpeedCameraOnRoute const & camera)
{
  // Clear previous speed cam in UI.
  m_speedCamClearCallback();

  m_currentHighlightedCamera = camera;
  m_speedCamShowCallback(camera.m_position, camera.m_maxSpeedKmH);
}

bool SpeedCameraManager::IsSpeedHigh(double distanceToCameraMeters, double speedMpS,
                                     SpeedCameraOnRoute const & camera) const
{
  if (camera.NoSpeed())
    return distanceToCameraMeters < kInfluenceZoneMeters + kDistToReduceSpeedBeforeUnknownCameraM;

  double const distToDangerousZone = distanceToCameraMeters - kInfluenceZoneMeters;

  if (distToDangerousZone < 0)
  {
    if (distToDangerousZone < -kInfluenceZoneMeters)
      return false;

    return speedMpS > routing::KMPH2MPS(camera.m_maxSpeedKmH);
  }

  if (speedMpS < routing::KMPH2MPS(camera.m_maxSpeedKmH))
    return false;

  double timeToSlowSpeed =
    (routing::KMPH2MPS(camera.m_maxSpeedKmH) - speedMpS) / kAverageAccelerationOfBraking;

  // Look to: https://en.wikipedia.org/wiki/Acceleration#Uniform_acceleration
  // S = V_0 * t + at^2 / 2, where
  //   V_0 - current speed
  //   a - kAverageAccelerationOfBraking
  double distanceNeedsToSlowDown = timeToSlowSpeed * speedMpS +
                                   (kAverageAccelerationOfBraking * timeToSlowSpeed * timeToSlowSpeed) / 2;
  distanceNeedsToSlowDown += kTimeForDecision * speedMpS;

  return distToDangerousZone < distanceNeedsToSlowDown + kDistanceEpsilonMeters;
}

bool SpeedCameraManager::SetNotificationFlags(double passedDistanceMeters, double speedMpS,
                                              SpeedCameraOnRoute const & camera)
{
  if (!Enable())
    return false;

  auto const distToCameraMeters = camera.m_distFromBeginMeters - passedDistanceMeters;

  Interval interval = SpeedCameraManager::GetIntervalByDistToCam(distToCameraMeters, speedMpS);
  switch (interval)
  {
  case Interval::ImpactZone:
  {
    SendEnterZoneStat(distToCameraMeters, speedMpS, camera);

    if (IsSpeedHigh(distToCameraMeters, speedMpS, camera))
    {
      m_makeBeepSignal = true;
      return true;
    }

    // If we did voice notification, and didn't beep signal in |BeepSignalZone|, let's do it now.
    // Only for Auto mode.
    if (m_voiceSignalCounter > 0 && m_beepSignalCounter == 0 &&
        m_mode == SpeedCameraManagerMode::Auto)
    {
      m_makeBeepSignal = true;
      return true;
    }

    if (m_mode == SpeedCameraManagerMode::Always)
    {
      m_makeVoiceSignal = true;
      return true;
    }

    return false;
  }
  case Interval::BeepSignalZone:
  {
    // If we exceeding speed limit, in |BeepSignalZone|, we should make "beep" signal.
    if (IsSpeedHigh(distToCameraMeters, speedMpS, camera))
    {
      m_makeBeepSignal = true;
      return true;
    }

    // If we did voice notification, we should do "beep" signal before |ImpactZone|.
    if (m_voiceSignalCounter > 0)
    {
      m_makeBeepSignal = true;
      return true;
    }

    return false;
  }
  case Interval::VoiceNotificationZone:
  {
    // If we exceeding speed limit, in |VoiceNotificationZone|, we should make "voice"
    // notification.
    if (IsSpeedHigh(distToCameraMeters, speedMpS, camera))
    {
      m_makeVoiceSignal = true;
      return true;
    }

    return false;
  }
  }

  UNREACHABLE();
}

bool SpeedCameraManager::NeedUpdateClosestCamera(double distanceToCameraMeters, double speedMpS,
                                                 SpeedCameraOnRoute const & camera)
{
  if (IsSpeedHigh(distanceToCameraMeters, speedMpS, camera))
    return true;

  if (m_mode == SpeedCameraManagerMode::Always && distanceToCameraMeters < kInfluenceZoneMeters)
    return true;

  return false;
}

bool SpeedCameraManager::IsHighlightedCameraExpired(double distToCameraMeters) const
{
  return distToCameraMeters < -kInfluenceZoneMeters;
}

bool SpeedCameraManager::NeedChangeHighlightedCamera(double distToCameraMeters,
                                                     bool needUpdateClosestCamera) const
{
  if (needUpdateClosestCamera)
    return true;

  if (!m_currentHighlightedCamera.IsValid() &&
      -kInfluenceZoneMeters < distToCameraMeters && distToCameraMeters < kShowCameraDistanceM)
  {
    return true;
  }

  return false;
}

void SpeedCameraManager::SendNotificationStat(double passedDistanceMeters, double speedMpS,
                                              SpeedCameraOnRoute const & camera)
{
  // Send stat about notification only when we are going to pronounce it.
  if (!BeepSignalAvailable() && !VoiceSignalAvailable())
    return;

  auto const distToCameraMeters = camera.m_distFromBeginMeters - passedDistanceMeters;

  ASSERT(m_makeBeepSignal != m_makeVoiceSignal, ("In each moment of time only one flag should be up."));
  alohalytics::TStringMap params = {{"type", m_makeBeepSignal ? "beep" : "voice"},
                                    {"distance", strings::to_string(distToCameraMeters)},
                                    {"speed", strings::to_string(measurement_utils::MpsToKmph(speedMpS))}};

  alohalytics::LogEvent("SpeedCameras_alert", params);
}

void SpeedCameraManager::SendEnterZoneStat(double distToCameraMeters, double speedMpS,
                                           SpeedCameraOnRoute const & camera)
{
  using strings::to_string;

  if (m_hasEnteredTheZone)
    return;
  m_hasEnteredTheZone = true;

  auto const latlon = MercatorBounds::ToLatLon(camera.m_position);
  alohalytics::TStringMap params = {
    {"distance", to_string(distToCameraMeters)},
    {"speed", to_string(measurement_utils::MpsToKmph(speedMpS))},
    {"speedlimit", camera.NoSpeed() ? "0" : to_string(camera.m_maxSpeedKmH)},
    {"coord", "(" + to_string(latlon.lat) + "," + to_string(latlon.lon) + ")"}
  };

  alohalytics::LogEvent("SpeedCameras_enter_zone", params);
}

void SpeedCameraManager::SetMode(SpeedCameraManagerMode mode)
{
  m_mode = mode;
  settings::Set(kSpeedCamModeKey, static_cast<int>(mode));
}

std::string DebugPrint(SpeedCameraManager::Interval interval)
{
  switch (interval)
  {
  case SpeedCameraManager::Interval::BeepSignalZone: return "BeepSignalZone";
  case SpeedCameraManager::Interval::VoiceNotificationZone: return "VoiceNotificationZone";
  case SpeedCameraManager::Interval::ImpactZone: return "ImpactZone";
  }
  
  UNREACHABLE();
}

std::string DebugPrint(SpeedCameraManagerMode mode)
{
  switch (mode)
  {
  case SpeedCameraManagerMode::Auto:
    return "auto";
  case SpeedCameraManagerMode::Always:
    return "always";
  case SpeedCameraManagerMode::Never:
    return "never";
  case SpeedCameraManagerMode::MaxValue:
    return "max_value";
  }

  UNREACHABLE();
}
}  // namespace routing
