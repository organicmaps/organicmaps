#pragma once

#include "platform/measurement_utils.hpp"

#include <string>

namespace generator
{
/// \brief Obtains |speed| and |units| by road category based on
/// the table in https://wiki.openstreetmap.org/wiki/Speed_limits
/// This method should be updated regularly. Last update: 22.10.18.
/// \note For example by passing string "RU:urban", you'll get
/// speed = 60 and units = Units::Metric.
/// \returns true if |speed| and |units| is found for |category| and these fields are filled
/// and false otherwise.
bool RoadCategoryToSpeed(std::string const & category, double & speed,
                         measurement_utils::Units & units);
}  // generator
