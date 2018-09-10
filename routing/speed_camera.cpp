#include "routing/speed_camera.hpp"

#include "routing/routing_helpers.hpp"

namespace routing
{
bool SpeedCameraOnRoute::IsDangerous(double distanceToCameraMeters, double speedMpS) const
{
  if (distanceToCameraMeters < kInfluenceZoneMeters + kDistanceEpsilonMeters)
    return true;

  if (m_maxSpeedKmH == kNoSpeedInfo)
    return distanceToCameraMeters < kInfluenceZoneMeters + kDistToReduceSpeedBeforeUnknownCameraM;

  double const distToDangerousZone = distanceToCameraMeters - kInfluenceZoneMeters;

  if (speedMpS < routing::KMPH2MPS(m_maxSpeedKmH))
    return false;

  double timeToSlowSpeed =
    (routing::KMPH2MPS(m_maxSpeedKmH) - speedMpS) / kAverageAccelerationOfBraking;

  // Look to: https://en.wikipedia.org/wiki/Acceleration#Uniform_acceleration
  // S = V_0 * t + at^2 / 2, where
  //   V_0 - current speed
  //   a - kAverageAccelerationOfBraking
  double distanceNeedsToSlowDown = timeToSlowSpeed * speedMpS +
                                   (kAverageAccelerationOfBraking * timeToSlowSpeed * timeToSlowSpeed) / 2;
  distanceNeedsToSlowDown += kTimeForDecision * speedMpS;

  if (distToDangerousZone < distanceNeedsToSlowDown + kDistanceEpsilonMeters)
    return true;

  return false;
}
}  // namespace routing
