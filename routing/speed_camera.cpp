#include "routing/speed_camera.hpp"

namespace routing
{
bool SpeedCameraOnRoute::IsValid() const
{
  return m_position != m2::PointD::Max();
}

void SpeedCameraOnRoute::Invalidate()
{
  m_position = m2::PointD::Max();
}

bool SpeedCameraOnRoute::NoSpeed() const
{
  return m_maxSpeedKmH == kNoSpeedInfo;
}
}  // namespace routing
