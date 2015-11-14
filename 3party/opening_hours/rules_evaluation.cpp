#include "rules_evaluation.hpp"
#include "rules_evaluation_private.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <tuple>

namespace
{
using THourMinutes = std::tuple<int, int>;
using osmoh::operator""_h;

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

// Transform timspan with extended end of the form of
// time less than 24 hours to extended form, i.e from 25 to 48 hours.
// Example: 12:15-06:00 -> 12:15-30:00.
inline void NormalizeExtendedEnd(osmoh::Timespan & span)
{
  auto & endHouminutes = span.GetEnd().GetHourMinutes();
  auto const duration = endHouminutes.GetDuration();
  if (duration < 24_h)
    endHouminutes.SetDuration(duration + 24_h);
}

inline osmoh::TTimespans SplitExtendedHours(osmoh::Timespan span)
{
  osmoh::TTimespans result;
  NormalizeExtendedEnd(span);

  auto spanToBeSplit = span;
  if (spanToBeSplit.HasExtendedHours())
  {
    osmoh::Timespan normalSpan;
    normalSpan.SetStart(spanToBeSplit.GetStart());
    normalSpan.SetEnd(osmoh::HourMinutes(24_h));
    result.push_back(normalSpan);

    spanToBeSplit.SetStart(osmoh::HourMinutes(0_h));
    spanToBeSplit.GetEnd().AddDuration(-24_h);
  }
  result.push_back(spanToBeSplit);

  return result;
}

inline void SplitExtendedHours(osmoh::TTimespans const & spans,
                               osmoh::TTimespans & originalNormilizedSpans,
                               osmoh::Timespan & additionalSpan)
{
  // We don't handle more than one occurence of extended span
  // since it is an invalid situation.
  for (auto it = begin(spans); it != end(spans); ++it)
  {
    if (!it->HasExtendedHours())
    {
      originalNormilizedSpans.push_back(*it);
      continue;
    }

    auto const splittedSpans = SplitExtendedHours(*it);
    originalNormilizedSpans.push_back(splittedSpans[0]);
    additionalSpan = splittedSpans[1];

    ++it;
    std::copy(it, end(spans), back_inserter(originalNormilizedSpans));

    break;
  }
}

inline bool HasExtendedHours(osmoh::RuleSequence const & rule)
{
  for (auto const & timespan : rule.GetTimes())
    if (timespan.HasExtendedHours())
      return true;

  return false;
}

std::tm MakeTimetuple(time_t const timestamp)
{
  std::tm tm{};
  localtime_r(&timestamp, &tm);
  return tm;
}
} // namespace

namespace osmoh
{
bool IsActive(Timespan const & span, std::tm const & time)
{
  // Timespan with e.h. should be split into parts with no e.h.
  // before calling IsActive().
  // TODO(mgsergio): set assert(!span.HasExtendedHours())

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

  return weekdays.GetWeekdayRanges().empty() &&
         weekdays.GetHolidays().empty();
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

bool IsActive(RuleSequence const & rule, time_t const timestamp)
{
  if (rule.IsTwentyFourHours())
    return true;

  auto const checkIsActive = [](RuleSequence const & rule, std::tm const & dt)
  {
    return IsActiveAny(rule.GetYears(), dt) && IsActiveAny(rule.GetMonths(), dt) &&
           IsActiveAny(rule.GetWeeks(), dt) && IsActive(rule.GetWeekdays(), dt);
  };

  auto const dateTimeTM = MakeTimetuple(timestamp);
  if (!HasExtendedHours(rule))
    return checkIsActive(rule, dateTimeTM) && IsActiveAny(rule.GetTimes(), dateTimeTM);

  TTimespans originalNormalizedSpans;
  Timespan additionalSpan;
  SplitExtendedHours(rule.GetTimes(), originalNormalizedSpans, additionalSpan);

  if (checkIsActive(rule, dateTimeTM) && IsActiveAny(originalNormalizedSpans, dateTimeTM))
    return true;

  time_t constexpr twentyFourHoursShift = 24 * 60 * 60;
  auto const dateTimeTMShifted = MakeTimetuple(timestamp - twentyFourHoursShift);

  if (checkIsActive(rule, dateTimeTMShifted) &&
      IsActive(additionalSpan, dateTimeTMShifted))
    return true;

  return false;
}

RuleState GetState(TRuleSequences const & rules, time_t const timestamp)
{
  auto emptyRuleIt = rules.rend();
  for (auto it = rules.rbegin(); it != rules.rend(); ++it)
  {
    if (IsActive(*it, timestamp))
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
} // namespace osmoh
