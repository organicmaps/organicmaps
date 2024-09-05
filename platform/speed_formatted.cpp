#include "speed_formatted.hpp"

#include "platform/locale.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"

namespace platform
{
using namespace measurement_utils;

SpeedFormatted::SpeedFormatted(double speedMps) : SpeedFormatted(speedMps, GetMeasurementUnits()) {}

SpeedFormatted::SpeedFormatted(double speedMps, Units units) : m_units(units)
{
  switch (units)
  {
    case Units::Metric:
      m_speed = MpsToKmph(speedMps);
      break;
    case Units::Imperial:
      m_speed = MpsToMiph(speedMps);
      break;
    default: UNREACHABLE();
  }
}

bool SpeedFormatted::IsValid() const { return m_speed >= 0.0; }

double SpeedFormatted::GetSpeed() const { return m_speed; }

Units SpeedFormatted::GetUnits() const { return m_units; }

std::string SpeedFormatted::GetSpeedString() const
{
  if (!IsValid())
    return "";

  // Default precision is 0 (no decimals).
  int precision = 0;

  // Set 1 decimal precision for speed per hour (km/h, miles/h) lower than 10.0 (9.5, 7.0,...).
  if (m_speed < 10.0)
    precision = 1;

  return ToStringPrecision(m_speed, precision);
}

std::string SpeedFormatted::GetUnitsString() const
{
  switch (m_units)
  {
  case Units::Metric: return GetLocalizedString("kilometers_per_hour");
  case Units::Imperial: return GetLocalizedString("miles_per_hour");
  default: UNREACHABLE();
  }
}

std::string SpeedFormatted::ToString() const
{
  if (!IsValid())
    return "";

  return GetSpeedString() + kNarrowNonBreakingSpace + GetUnitsString();
}

}  // namespace platform
