#include "rules_evaluation.hpp"
#include "rules_evaluation_private.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <tuple>

namespace
{
using THourMinutes = std::tuple<int, int>;

constexpr osmoh::MonthDay::TYear kTMYearOrigin = 1900;

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
    // TODO(mgsergio): Not implemented yet
    return false;

  if (monthDay.HasYear())
    if (monthDay.GetYear() != date.tm_year + kTMYearOrigin)
      return monthDay.GetYear() != date.tm_year + kTMYearOrigin;

  if (monthDay.HasMonth())
    if (monthDay.GetMonth() != osmoh::ToMonth(date.tm_mon + 1))
      return static_cast<int>(monthDay.GetMonth()) - (date.tm_mon + 1);

  if (monthDay.HasDayNum())
    if (monthDay.GetDayNum() != date.tm_mday)
      return monthDay.GetDayNum() - date.tm_mday;

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

// Fill result with fields that present in start and missing in end.
osmoh::MonthDay NormalizeEnd(osmoh::MonthDay const & start, osmoh::MonthDay const & end)
{
  osmoh::MonthDay result = start;
  if (end.HasYear())
    result.SetYear(end.GetYear());
  if (end.HasMonth())
    result.SetMonth(end.GetMonth());
  if (end.HasDayNum())
    result.SetDayNum(end.GetDayNum());
  return result;
}

uint8_t GetWeekNumber(std::tm const & date)
{
  char buff[4]{};
  if (strftime(&buff[0], sizeof(buff), "%V", &date) == 0)
    return 0;

  uint32_t weekNumber;
  std::stringstream sstr(buff);
  sstr >> weekNumber;
  return weekNumber;
}

bool IsBetweenLooped(osmoh::Weekday const start,
                     osmoh::Weekday const end,
                     osmoh::Weekday const p)
{
  if (start <= end)
    return start <= p && p <= end;
  return p >= end || start <= p;
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
    return IsBetweenLooped(range.GetStart(), range.GetEnd(), wday);

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

  return
      weekdays.GetWeekdayRanges().empty() &&
      weekdays.GetHolidays().empty();
}

bool IsActive(MonthdayRange const & range, std::tm const & date)
{
  if (range.IsEmpty())
    return false;

  if (range.HasEnd())
    return
        range.GetStart() <= date &&
        date <= NormalizeEnd(range.GetStart(), range.GetEnd());

  return range.GetStart() == date;
}

bool IsActive(YearRange const & range, std::tm const & date)
{
  if (range.IsEmpty())
    return false;

  auto const year = date.tm_year + kTMYearOrigin;

  if (range.HasEnd())
    return range.GetStart() <= year && year <= range.GetEnd();

  return range.GetStart() == year;
}

bool IsActive(WeekRange const & range, std::tm const & date)
{
  if (range.IsEmpty())
    return false;

  auto const weekNumber = GetWeekNumber(date);

  if (range.HasEnd())
    return range.GetStart() <= weekNumber && weekNumber <= range.GetEnd();

  return range.GetStart() == weekNumber;
}

template <typename T>
bool IsActiveAny(std::vector<T> const & selectors, std::tm const & date)
{
  for (auto const & selector : selectors)
    if (IsActive(selector, date))
      return true;

  return selectors.empty();
}

bool IsActive(RuleSequence const & rule, std::tm const & date)
{
  if (rule.IsTwentyFourHours())
    return true;

  return
      IsActiveAny(rule.GetYears(), date) &&
      IsActiveAny(rule.GetMonths(), date) &&
      IsActiveAny(rule.GetWeeks(), date) &&
      IsActive(rule.GetWeekdays(), date) &&
      IsActiveAny(rule.GetTimes(), date);
}

RuleState GetState(TRuleSequences const & rules, std::tm const & date)
{
  auto emptyRuleIt = rules.rend();
  for (auto it = rules.rbegin(); it != rules.rend(); ++it)
  {
    if (IsActive(*it, date))
    {
      if (it->IsEmpty() && emptyRuleIt == rules.rend())
        emptyRuleIt = it;
      else
        return it->GetModifier();
    }
  }

  if (emptyRuleIt != rules.rend())
  {
    if (emptyRuleIt->HasComment())
      return RuleSequence::Modifier::Unknown;
    else
      return emptyRuleIt->GetModifier();
  }

  return (rules.empty()
          ? RuleSequence::Modifier::Unknown
          : RuleSequence::Modifier::Closed);
}

RuleState GetState(TRuleSequences const & rules, time_t const dateTime)
{
  std::tm tm{};
  localtime_r(&dateTime, &tm);
  return GetState(rules, tm);
}
} // namespace osmoh
