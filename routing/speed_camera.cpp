#include "routing/speed_camera.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/speed_camera_manager.hpp"
#include "speed_camera.hpp"

namespace routing
{
bool SpeedCameraOnRoute::IsValid() const
{
  return m_position != m2::PointD::Zero();
}

void SpeedCameraOnRoute::Invalidate()
{
  m_position = m2::PointD::Zero();
}
}  // namespace routing
