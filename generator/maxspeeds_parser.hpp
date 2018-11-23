#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include <cstdint>
#include <string>

namespace generator
{
/// \brief Obtains |speed| and |units| by road category based on
/// the table at https://wiki.openstreetmap.org/wiki/Speed_limits
/// This function should be updated regularly. Last update: 23.10.18.
/// \returns true if |speed| and |units| is found for |category| and these fields are filled
/// and false otherwise.
/// \note For example by passing string "RU:urban", you'll get
/// speed = 60 and units = Units::Metric.
/// \note If the method returns true the field |speed| may be filled with |kNoneMaxSpeed| value.
/// It means no speed limits for the |category|. It is currently the case for some road categories
/// in Germany.
bool RoadCategoryToSpeed(std::string const & category, routing::SpeedInUnits & speed);

/// \brief Tries to convert the value of tag maxspeed to speed in appropriate units.
/// \returns false in case of error and true if the conversion is successful.
/// \note speed.m_speed will be filled with kNoneMaxSpeed or kWalkMaxSpeed if
/// maxspeedValue is equal to "none" or "walk". The value of speed.m_units does not
/// matter in this case.
bool ParseMaxspeedTag(std::string const & maxspeedValue, routing::SpeedInUnits & speed);

std::string UnitsToString(measurement_utils::Units units);

/// \brief Converts string to measurement_utils::Units.
/// \note |units| should be "Metric" or "Imperial".
measurement_utils::Units StringToUnits(std::string const & units);
}  // namespace generator
