#include "rules_evaluation.hpp"
// #include "rules_evaluation_private.hpp"
#include <tuple>

namespace
{
using THourMinutes = std::tuple<int, int>;

bool ToHourMinutes(osmoh::Time const & t, THourMinutes & hm)
{
  if (!t.IsHoursMinutes())
    return false;
  hm = THourMinutes{t.GetHoursCount(), t.GetMinutesCount()};
  return true;
}

bool ToHourMinutes(std::tm const & t, THourMinutes & hm)
{
  hm = THourMinutes{t.tm_hour, t.tm_min};
  return true;
}

int CompareMonthDayAndTimeTumple(osmoh::MonthDay const & monthDay, std::tm const & date)
{
  if (monthDay.IsVariable())
    // Not implemented yet
    return false;

  constexpr osmoh::MonthDay::TYear kTMYearOrigin = 1900;
  if (monthDay.HasYear())
    if (monthDay.GetYear() != date.tm_year + kTMYearOrigin)
      return monthDay.GetYear() != date.tm_year + kTMYearOrigin;

  if (monthDay.HasMonth())
    if (monthDay.GetMonth() != osmoh::ToMonth(date.tm_mon + 1))
      return static_cast<int>(monthDay.GetMonth()) - (date.tm_mon + 1);

  return 0;
}

bool operator<=(osmoh::MonthDay const & monthDay, std::tm const & date)
{
  return CompareMonthDayAndTimeTumple(monthDay, date) < 1;
}

bool operator<=(std::tm const & date, osmoh::MonthDay const & monthDay)
{
  return CompareMonthDayAndTimeTumple(monthDay, date) > -1;
}

bool operator==(osmoh::MonthDay const & monthDay, std::tm const & date)
{
  return CompareMonthDayAndTimeTumple(monthDay, date) == 0;
}


osmoh::MonthDay NormalizeEnd(osmoh::MonthDay const & start, osmoh::MonthDay const & end)
{
  osmoh::MonthDay result = start;
  if (end.HasYear())
    result.SetYear(end.GetYear());
  if (end.HasMonth())
    result.SetMonth(end.GetMonth());
  return result;
}
} // namespace


namespace osmoh
{
bool IsActive(Timespan const & span, std::tm const & time)
{
  if (span.HasStart() && span.HasEnd())
  {
    THourMinutes start;
    THourMinutes end;
    THourMinutes toBeChecked;
    if (!ToHourMinutes(span.GetStart(), start))
      return false;
    if (!ToHourMinutes(span.GetEnd(), end))
      return false;
    if (!ToHourMinutes(time, toBeChecked))
      return false;

    return start <= toBeChecked && toBeChecked <= end;

  }
  return false;
}

bool IsActive(WeekdayRange const & range, std::tm const & date)
{
  if (range.IsEmpty())
    return false;

  auto const wday = ToWeekday(date.tm_wday + 1);
  if (wday == Weekday::None)
    return false;

  if (range.HasEnd())
    return range.GetStart() <= wday && wday <= range.GetEnd();
  return range.GetStart() == wday;
}

bool IsActive(Holiday const & holiday, std::tm const & date)
{
  return false;
}

bool IsActive(Weekdays const & weekdays, std::tm const & date)
{
  for (auto const & wr : weekdays.GetWeekdayRanges())
    if (IsActive(wr, date))
      return true;

  for (auto const & hd : weekdays.GetHolidays())
    if (IsActive(hd, date))
      return true;

  return false;
}

bool IsActive(MonthdayRange const & range, std::tm const & date)
{
  if (range.IsEmpty())
    return false;

  if (range.HasEnd())
    return range.GetStart() <= date &&
        date <= NormalizeEnd(range.GetStart(), range.GetEnd());

  return range.GetStart() == date;
}
} // namespace osmoh
