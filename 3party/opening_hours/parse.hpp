#pragma once

#include "osm_time_range.hpp"
#include <string>

namespace osmoh
{
bool Parse(std::string const &, TTimespans &);
bool Parse(std::string const &, Weekdays &);
bool Parse(std::string const &, TMonthdayRanges &);
bool Parse(std::string const &, TYearRanges &);
} // namespace osmoh
