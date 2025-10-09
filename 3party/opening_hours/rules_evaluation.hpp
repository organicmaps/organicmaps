#pragma once

#include "opening_hours.hpp"

#include <ctime>

namespace osmoh
{

RuleState GetState(TRuleSequences const & rules, time_t const dateTime, std::string const & countryId);

time_t GetNextTimeState(TRuleSequences const & rules, time_t const dateTime, RuleState state, std::string const & countryId);

inline bool IsOpen(TRuleSequences const & rules, time_t const dateTime, std::string const & countryId)
{
  return GetState(rules, dateTime, countryId) == RuleState::Open;
}

inline time_t GetNextTimeOpen(TRuleSequences const & rules, time_t const dateTime, std::string const & countryId ="")
{
  if (GetState(rules, dateTime, countryId) == RuleState::Open)
    return dateTime;
  return GetNextTimeState(rules, dateTime, RuleState::Open, countryId);
}

inline bool IsClosed(TRuleSequences const & rules, time_t const dateTime, std::string const & countryId)
{
  return GetState(rules, dateTime, countryId) == RuleState::Closed;
}

inline time_t GetNextTimeClosed(TRuleSequences const & rules, time_t const dateTime, std::string const & countryId ="")
{
  if (GetState(rules, dateTime, countryId) == RuleState::Closed)
    return dateTime;
  return GetNextTimeState(rules, dateTime, RuleState::Closed, countryId);
}

inline bool IsUnknown(TRuleSequences const & rules, time_t const dateTime, std::string const & countryId)
{
  return GetState(rules, dateTime, countryId) == RuleState::Unknown;
}

} // namespace osmoh
