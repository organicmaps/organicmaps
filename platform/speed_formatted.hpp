#pragma once

#include "platform/measurement_utils.hpp"

#include <string>

namespace platform
{
class SpeedFormatted
{
public:
  SpeedFormatted(double speedInMps); // Initialize with m/s value and default measurement units
  SpeedFormatted(double speedInMps, measurement_utils::Units units);

  bool IsValid() const;

  double GetSpeed() const;
  measurement_utils::Units GetUnits() const;

  std::string GetSpeedString() const;
  std::string GetUnitsString() const;

  std::string ToString() const;

  friend std::string DebugPrint(SpeedFormatted const & d) { return d.ToString(); }

private:
  double m_speed; // Speed in km/h or mile/h depending on m_units.
  measurement_utils::Units m_units;
};

}  // namespace platform
