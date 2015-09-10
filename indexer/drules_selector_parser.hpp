#pragma once

#include "std/string.hpp"

namespace drule
{

enum SelectorOperatorType
{
  SelectorOperatorUnknown = 0,
  SelectorOperatorNotEqual, // [tag!=value]
  SelectorOperatorLessOrEqual, // [tag<=value]
  SelectorOperatorGreaterOrEqual, // [tag>=value]
  SelectorOperatorEqual, // [tag=value]
  SelectorOperatorLess, // [tag<value]
  SelectorOperatorGreater, // [tag>value]
  SelectorOperatorIsNotSet, // [!tag]
  SelectorOperatorIsSet, // [tag]
};

struct SelectorExpression
{
  SelectorOperatorType m_operator;
  string m_tag;
  string m_value;

  SelectorExpression() : m_operator(SelectorOperatorUnknown) {}
};

bool ParseSelector(string const & str, SelectorExpression & e);

}  // namespace drule
