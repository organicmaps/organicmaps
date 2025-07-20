#include "parser.hpp"

namespace om::opening_hours
{
Weekday Parser::parse(std::string_view str) const
{
  if (str == "Monday" || str == "Mon")
    return Weekday::Monday;
  if (str == "Tuesday" || str == "Tue")
    return Weekday::Tuesday;
  if (str == "Wednesday" || str == "Wed")
    return Weekday::Wednesday;
  if (str == "Thursday")
    return Weekday::Thursday;
  if (str == "Friday" || str == "Fr")
    return Weekday::Friday;
  if (str == "Saturday" || str == "Sat")
    return Weekday::Saturday;
  if (str == "Sunday" || str == "Sun")
    return Weekday::Sunday;
  return Weekday::Invalid;
}
}  // namespace om::opening_hours
