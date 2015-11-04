#pragma once

#include "opening_hours.hpp"
#include <ctime>

namespace osmoh
{
class RuleState
{
public:
  RuleState(RuleSequence::Modifier const & modifier):
      m_modifier(modifier)
  {
  }

  operator bool() const
  {
    return IsOpen();
  }

  bool IsOpen() const
  {
    return
        m_modifier == RuleSequence::Modifier::DefaultOpen ||
        m_modifier == RuleSequence::Modifier::Open;
  }

  bool IsClosed() const
  {
    return m_modifier == RuleSequence::Modifier::Closed;
  }

  bool IsUnknown() const
  {
    return
        m_modifier == RuleSequence::Modifier::Unknown ||
        m_modifier == RuleSequence::Modifier::Comment;
  }

private:
  RuleSequence::Modifier m_modifier;
};

RuleState GetState(TRuleSequences const & rules, std::tm const & date);
RuleState GetState(TRuleSequences const & rules, time_t const dateTime);
} // namespace osmoh
