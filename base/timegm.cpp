#include "timegm.hpp"

// There are issues with this implementation due to absence
// of time_t fromat specification. There are no guarantees
// of its internal structure so we cannot rely on + or -.

// It could be possible to substitute time_t with boost::ptime
// in contexts where we need some arithmetical and logical operations
// on time and we don't have to use precisely time_t.

namespace
{
// Number of days elapsed since Jan 01 up to each month
// (except for February in leap years).
int const g_monoff[] = {
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

bool IsLeapYear(int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int LeapDaysCount(int y1, int y2)
{
  --y1;
  --y2;
  return (y2/4 - y1/4) - (y2/100 - y1/100) + (y2/400 - y1/400);
}
} // namespace

namespace base
{
// Inspired by python's calendar.py
time_t TimeGM(std::tm const & tm)
{
  int year;
  time_t days;
  time_t hours;
  time_t minutes;
  time_t seconds;

  year = 1900 + tm.tm_year;
  days = 365 * (year - 1970) + LeapDaysCount(1970, year);
  days += g_monoff[tm.tm_mon];

  if (tm.tm_mon > 1 && IsLeapYear(year))
    ++days;
  days += tm.tm_mday - 1;

  hours = days * 24 + tm.tm_hour;
  minutes = hours * 60 + tm.tm_min;
  seconds = minutes * 60 + tm.tm_sec;

  return seconds;
}
} // namespace base
