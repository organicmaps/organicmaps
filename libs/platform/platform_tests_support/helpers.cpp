#include "helpers.hpp"

#include "base/assert.hpp"

#include <ctime>

namespace platform::tests_support
{
time_t GetUnixtimeByDate(uint16_t year, Month month, uint8_t monthDay, uint8_t hours, uint8_t minutes)
{
  std::tm t{};
  t.tm_year = year - 1900;
  t.tm_mon = static_cast<int>(month) - 1;
  t.tm_mday = monthDay;
  t.tm_hour = hours;
  t.tm_min = minutes;
  return std::mktime(&t);
}

time_t GetUnixtimeByWeekday(uint16_t year, Month month, Weekday weekday, uint8_t hours, uint8_t minutes)
{
  int monthDay = 1;
  auto createUnixtime = [&]()
  {
    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon = static_cast<int>(month) - 1;
    t.tm_mday = monthDay;
    t.tm_wday = static_cast<int>(weekday) - 1;
    t.tm_hour = hours;
    t.tm_min = minutes;
    return std::mktime(&t);
  };

  for (; monthDay < 32; ++monthDay)
  {
    auto const unixtime = createUnixtime();
    auto const timeOut = std::localtime(&unixtime);
    if (timeOut->tm_wday == static_cast<int>(weekday) - 1)
      return unixtime;
  }

  UNREACHABLE();
}
}  // namespace platform::tests_support
