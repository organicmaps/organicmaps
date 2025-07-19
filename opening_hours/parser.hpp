#pragma once

#include <string_view>

#include "weekday.hpp"

namespace om::opening_hours
{
class Parser
{
public:
  Weekday parse(std::string_view str) const;
};
}  // namespace om::opening_hours
