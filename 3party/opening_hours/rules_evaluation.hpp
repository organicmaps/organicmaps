#pragma once

#include "opening_hours.hpp"

#include <ctime>

namespace osmoh
{

RuleState GetState(TRuleSequences const & rules, time_t const dateTime, THolidayDates const & holidays = {});

time_t GetNextTimeState(TRuleSequences const & rules, time_t const dateTime, RuleState state, THolidayDates const & holidays = {});

inline bool IsOpen(TRuleSequences const & rules, time_t const dateTime, THolidayDates const & holidays = {})
{
  return GetState(rules, dateTime, holidays) == RuleState::Open;
}

inline time_t GetNextTimeOpen(TRuleSequences const & rules, time_t const dateTime, THolidayDates const & holidays = {})
{
  if (GetState(rules, dateTime, holidays) == RuleState::Open)
    return dateTime;
  return GetNextTimeState(rules, dateTime, RuleState::Open, holidays);
}

inline bool IsClosed(TRuleSequences const & rules, time_t const dateTime, THolidayDates const & holidays = {})
{
  return GetState(rules, dateTime, holidays) == RuleState::Closed;
}

inline time_t GetNextTimeClosed(TRuleSequences const & rules, time_t const dateTime, THolidayDates const & holidays = {})
{
  if (GetState(rules, dateTime, holidays) == RuleState::Closed)
    return dateTime;
  return GetNextTimeState(rules, dateTime, RuleState::Closed, holidays);
}

inline bool IsUnknown(TRuleSequences const & rules, time_t const dateTime, THolidayDates const & holidays = {})
{
  return GetState(rules, dateTime, holidays) == RuleState::Unknown;
}

} // namespace osmoh
