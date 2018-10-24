#pragma once

#include "platform/measurement_utils.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace generator
{
uint16_t constexpr kNoneMaxSpeed = std::numeric_limits<uint16_t>::max();
uint16_t constexpr kWalkMaxSpeed = std::numeric_limits<uint16_t>::max() - 1;

struct SpeedInUnits
{
  SpeedInUnits() = default;
  SpeedInUnits(uint16_t speed, measurement_utils::Units units) noexcept : m_speed(speed), m_units(units) {}

  bool operator==(SpeedInUnits const & s) const { return m_speed == s.m_speed && m_units == s.m_units; }

  bool IsNumeric() const { return m_speed != kNoneMaxSpeed && m_speed != kWalkMaxSpeed; }

  uint16_t m_speed = 0; // Speed in km per hour or mile per hour depends on m_units value.
  measurement_utils::Units m_units = measurement_utils::Units::Metric;
};

/// \brief Obtains |speed| and |units| by road category based on
/// the table in https://wiki.openstreetmap.org/wiki/Speed_limits
/// This method should be updated regularly. Last update: 23.10.18.
/// \returns true if |speed| and |units| is found for |category| and these fields are filled
/// and false otherwise.
/// \note For example by passing string "RU:urban", you'll get
/// speed = 60 and units = Units::Metric.
/// \note If the method returns true the field |speed| may be filled with |kNoneMaxSpeed| value.
/// It means no speed limits for the |category|. It's currently actual for Germany.
bool RoadCategoryToSpeed(std::string const & category, SpeedInUnits & speed);

/// \brief Tries to convert the value of tag maxspeed to speed in appropriate units.
/// \returns false in case of error and true if the conversion is successful.
/// \note speed.m_speed will be filled with kNoneMaxSpeed or kWalkMaxSpeed if
/// maxspeedValue is equal to "none" or "walk". The value of speed.m_units does not
/// matter in this case.
bool MaxspeedValueToSpeed(std::string const & maxspeedValue, SpeedInUnits & speed);

std::string DebugPrint(SpeedInUnits const & speed);
}  // generator
