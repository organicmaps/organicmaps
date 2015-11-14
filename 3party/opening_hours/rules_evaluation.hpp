#pragma once

#include "opening_hours.hpp"

#include <ctime>

namespace osmoh
{
enum class RuleState
{
  Open,
  Closed,
  Unknown
};

RuleState GetState(TRuleSequences const & rules, time_t const dateTime);

inline bool IsOpen(TRuleSequences const & rules, time_t const dateTime)
{
  return GetState(rules, dateTime) == RuleState::Open;
}

inline bool IsClosed(TRuleSequences const & rules, time_t const dateTime)
{
  return GetState(rules, dateTime) == RuleState::Closed;
}

inline bool IsUnknown(TRuleSequences const & rules, time_t const dateTime)
{
  return GetState(rules, dateTime) == RuleState::Unknown;
}
} // namespace osmoh
