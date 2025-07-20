#include "weekday.hpp"

namespace om::opening_hours
{
std::string DebugPrint(Weekday weekday)
{
  if (weekday == Weekday::Monday)
    return "Monday";
  if (weekday == Weekday::Tuesday)
    return "Tuesday";
  if (weekday == Weekday::Wednesday)
    return "Wednesday";
  if (weekday == Weekday::Thursday)
    return "Thursday";
  if (weekday == Weekday::Friday)
    return "Friday";
  if (weekday == Weekday::Saturday)
    return "Saturday";
  if (weekday == Weekday::Sunday)
    return "Sunday";
  return "Invalid";
}
}  // namespace om::opening_hours
