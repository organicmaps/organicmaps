#include "base/timegm.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"

#include <chrono>
#ifndef OMIM_OS_WINDOWS_NATIVE
#include <time.h>  // timegm is a non-POSIX, non-standard function.
#endif

// There are issues with this implementation due to absence
// of time_t format specification. There are no guarantees
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

int LeapDaysCount(int y1, int y2)
{
  --y1;
  --y2;
  return (y2/4 - y1/4) - (y2/100 - y1/100) + (y2/400 - y1/400);
}
} // namespace

namespace base
{
int DaysOfMonth(int year, int month)
{
  ASSERT_GREATER_OR_EQUAL(month, 1, ());
  ASSERT_LESS_OR_EQUAL(month, 12, ());

  int const february = base::IsLeapYear(year) ? 29 : 28;
  int const daysPerMonth[12] = { 31, february, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return daysPerMonth[month - 1];
}

bool IsLeapYear(int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Inspired by python's calendar.py
time_t TimeGM(std::tm & tm)
{
#ifdef OMIM_OS_WINDOWS_NATIVE
  return _mkgmtime(&tm);
#else
  return timegm(&tm);
#endif
}

time_t TimeGM(int year, int month, int day, int hour, int min, int sec)
{
  ASSERT_GREATER_OR_EQUAL(month, 1, ());
  ASSERT_LESS_OR_EQUAL(month, 12, ());
  ASSERT_GREATER_OR_EQUAL(day, 1, ());
  ASSERT_LESS_OR_EQUAL(day, DaysOfMonth(year, month), ());

  tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  return TimeGM(t);
}
} // namespace base
