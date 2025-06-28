#pragma once

#include <string>

namespace om::opening_hours
{
enum class Weekday : uint8_t
{
  Monday = 0,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
  Saturday,
  Sunday,
  Invalid = 255
};

std::string DebugPrint(Weekday weekday);
}  // namespace om::opening_hours
