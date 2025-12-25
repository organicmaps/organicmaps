#pragma once

#include "timezone/timezone.hpp"

namespace om::tz
{
std::string Serialize(TimeZone const & timeZone);
TimeZone Deserialize(std::string_view const & data);
}  // namespace om::tz
