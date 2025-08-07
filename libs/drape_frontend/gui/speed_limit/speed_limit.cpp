#include "speed_limit.hpp"

#include "platform/measurement_utils.hpp"

namespace gui::speed_limit
{
void SpeedLimit::SetSpeedLimit(double speedLimitMps)
{
  m_speedLimitMps = speedLimitMps;
}

bool SpeedLimit::IsSpeedLimitAvailable() const
{
  return true;
}

std::string SpeedLimit::GetSpeedLimit() const
{
  return "120";
  return measurement_utils::FormatSpeedNumeric(m_speedLimitMps, measurement_utils::GetMeasurementUnits());
}
}  // namespace gui::speed_limit
