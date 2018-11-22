#include "routing/speed_camera_manager.hpp"

#include "routing/speed_camera.hpp"
#include "speed_camera_manager.hpp"


namespace routing
{
std::string const SpeedCameraManager::kSpeedCamModeKey = "speed_cam_mode";

SpeedCameraManager::SpeedCameraManager(turns::sound::NotificationManager & notificationManager)
  : m_notificationManager(notificationManager)
{
  Reset();

  uint32_t mode = 0;
  if (settings::Get(kSpeedCamModeKey, mode) &&
      mode < static_cast<int>(SpeedCameraManagerMode::MaxValue))
  {
    m_mode = static_cast<SpeedCameraManagerMode>(mode);
  }
  else
  {
    m_mode = SpeedCameraManagerMode::Auto;
  }

  //tmp code
  m_mode = SpeedCameraManagerMode::Auto;
  //end tmp code
}

//static
SpeedCameraManager::Interval
SpeedCameraManager::GetIntervalByDistToCam(double distanceToCameraMeters, double speedMpS)
{
  Interval interval;
  if (distanceToCameraMeters < kInfluenceZoneMeters)
  {
    interval = Interval::ImpactZone;
  }
  else
  {
    double beepDist = kBeepSignalTime * speedMpS;
    if (distanceToCameraMeters < kInfluenceZoneMeters + beepDist)
      interval = Interval::BeepSignalZone;
    else
      interval = Interval::VoiceNotificationZone;
  }

  return interval;
}

void SpeedCameraManager::OnLocationPositionChanged(location::GpsInfo const & info)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!Enable())
    return;

  CHECK(!m_route.expired(), ());

  auto const passedDistanceMeters = m_route.lock()->GetCurrentDistanceFromBeginMeters();

  // Step 1. Find new cameras and cache them.
  FindCamerasOnRouteAndCache(passedDistanceMeters);

  double distFromCurrentPosAndClosestCam = -1.0;
  if (m_closestCamera.IsValid())
    distFromCurrentPosAndClosestCam = m_closestCamera.m_distFromBeginMeters - passedDistanceMeters;

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

  if (m_closestCamera.IsValid())
    SetNotificationFlags(passedDistanceMeters, info.m_speedMpS, m_closestCamera);

  // Step 3. Check UI camera (stop or not stop to highlight it).
  if (m_currentHighlightedCamera.IsValid())
  {
    auto const distToCameraMeters =
      m_currentHighlightedCamera.m_distFromBeginMeters - passedDistanceMeters;

    if (IsHighlightedCameraExpired(distToCameraMeters))
    {
      m_speedCamClearCallback();
      m_currentHighlightedCamera.Invalidate();
    }
  }
}

void SpeedCameraManager::GenerateNotifications(std::vector<std::string> & notifications)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!Enable())
    return;

  if (m_makeVoiceSignal && m_voiceSignalCounter < kVoiceNotificationNumber)
  {
    notifications.emplace_back(m_notificationManager.GenerateSpeedCameraText());
    m_makeVoiceSignal = false;
    ++m_voiceSignalCounter;
  }
}

bool SpeedCameraManager::ShouldPlayWarningSignal()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!Enable())
    return false;

  if (m_makeBeepSignal && m_beepSignalCounter < kBeepSignalNumber)
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
  m_beepSignalCounter = 0;
  m_voiceSignalCounter = 0;
}

void SpeedCameraManager::Reset()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_closestCamera.Invalidate();
  m_firstNotCheckedSpeedCameraIndex = 1;
  ResetNotifications();
  m_speedCamClearCallback();
  m_cachedSpeedCameras = std::queue<SpeedCameraOnRoute>();
}

void SpeedCameraManager::FindCamerasOnRouteAndCache(double passedDistanceMeters)
{
  CHECK(Enable(), ("Speed camera manager is off."));
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

bool SpeedCameraManager::IsSpeedHigh(double distanceToCameraMeters, double speedMpS,
                                     SpeedCameraOnRoute const & camera) const
{
  if (camera.NoSpeed())
    return distanceToCameraMeters < kInfluenceZoneMeters + kDistToReduceSpeedBeforeUnknownCameraM;

  double const distToDangerousZone = distanceToCameraMeters - kInfluenceZoneMeters;

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
  CHECK(m_mode != SpeedCameraManagerMode::Never,
        ("This method should use only in Auto and Always mode."));

  auto const distToCameraMeters = camera.m_distFromBeginMeters - passedDistanceMeters;

  Interval interval = SpeedCameraManager::GetIntervalByDistToCam(distToCameraMeters, speedMpS);
  switch (interval)
  {
    case Interval::ImpactZone:
    {
      if (IsSpeedHigh(distToCameraMeters, speedMpS, camera))
      {
        m_makeBeepSignal = true;
        return true;
      }

      // If we did voice notification, and didn't beep signal in |BeepSignalZone|, let's do it now.
      if (m_voiceSignalCounter > 0 && m_beepSignalCounter == 0)
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

  CHECK_SWITCH();
  return false;
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
}  // namespace routing
