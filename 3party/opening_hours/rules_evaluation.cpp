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

inline bool ToHourMinutes(osmoh::Time const & t, THourMinutes & hm)
{
  if (!t.IsHoursMinutes())
    return false;
  hm = THourMinutes{t.GetHoursCount(), t.GetMinutesCount()};
  return true;
}

inline bool ToHourMinutes(std::tm const & t, THourMinutes & hm)
{
  hm = THourMinutes{t.tm_hour, t.tm_min};
  return true;
}

inline int CompareMonthDayTimeTuple(osmoh::MonthDay const & monthDay, std::tm const & date)
{
  if (monthDay.HasYear())
  {
    if (monthDay.GetYear() != date.tm_year + kTMYearOrigin)
      return monthDay.GetYear() != date.tm_year + kTMYearOrigin;
  }

  if (monthDay.HasMonth())
  {
    if (monthDay.GetMonth() != osmoh::ToMonth(date.tm_mon + 1))
      return static_cast<int>(monthDay.GetMonth()) - (date.tm_mon + 1);
  }

  if (monthDay.HasDayNum())
  {
    if (monthDay.GetDayNum() != date.tm_mday)
      return monthDay.GetDayNum() - date.tm_mday;
  }

  return 0;
}

inline bool operator<=(osmoh::MonthDay const & monthDay, std::tm const & date)
{
  return CompareMonthDayTimeTuple(monthDay, date) < 1;
}

inline bool operator<=(std::tm const & date, osmoh::MonthDay const & monthDay)
{
  return CompareMonthDayTimeTuple(monthDay, date) > -1;
}

inline bool operator==(osmoh::MonthDay const & monthDay, std::tm const & date)
{
  return CompareMonthDayTimeTuple(monthDay, date) == 0;
}

// Fill result with fields that present in start and missing in end.
inline osmoh::MonthDay NormalizeEnd(osmoh::MonthDay const & start, osmoh::MonthDay const & end)
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

inline uint8_t GetWeekNumber(std::tm const & date)
{
  char buff[4]{};
  if (strftime(&buff[0], sizeof(buff), "%V", &date) == 0)
    return 0;

  uint32_t weekNumber;
  std::stringstream sstr(buff);
  sstr >> weekNumber;
  return weekNumber;
}

inline bool IsBetweenLooped(osmoh::Weekday const start,
                            osmoh::Weekday const end,
                            osmoh::Weekday const p)
{
  if (start <= end)
    return start <= p && p <= end;
  return p >= end || start <= p;
}

inline osmoh::RuleState ModifierToRuleState(osmoh::RuleSequence::Modifier const modifier)
{
  using Modifier = osmoh::RuleSequence::Modifier;

  switch(modifier)
  {
    case Modifier::DefaultOpen:
    case Modifier::Open:
      return osmoh::RuleState::Open;

    case Modifier::Closed:
      return osmoh::RuleState::Closed;

    case Modifier::Unknown:
    case Modifier::Comment:
      return osmoh::RuleState::Unknown;
  }
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

    // TODO(mgsergio): We don't handle extended hours yet.
    // Extended hours handling could be implemented through
    // splitting rule with extended hours into separated rules.
    // See https://trello.com/c/Efsvs6PP/23-opening-hours-extended-hours
    if (end <= start || end > THourMinutes{24, 00})
      // It's better to say we are open, cause
      // in search result page only `closed' lables are shown.
      // So from user's perspective `unknown' and `open'
      // mean same as `not closed'.
      return true;

    return start <= toBeChecked && toBeChecked <= end;
  }
  else if (span.HasStart() && span.HasPlus())
  {
    // TODO(mgsergio): Not implemented yet
    return false;
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
  {
    if (IsActive(selector, date))
      return true;
  }

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
        return ModifierToRuleState(it->GetModifier());
    }
  }

  if (emptyRuleIt != rules.rend())
  {
    if (emptyRuleIt->HasComment())
      return RuleState::Unknown;
    else
      return ModifierToRuleState(emptyRuleIt->GetModifier());
  }

  return (rules.empty()
          ? RuleState::Unknown
          : RuleState::Closed);
}

RuleState GetState(TRuleSequences const & rules, time_t const dateTime)
{
  std::tm tm{};
  localtime_r(&dateTime, &tm);
  return GetState(rules, tm);
}
} // namespace osmoh
