#pragma once

#include <string>

namespace drule
{

enum SelectorOperatorType
{
  SelectorOperatorUnknown = 0,

  // [tag!=value]
  SelectorOperatorNotEqual,

  // [tag<=value]
  SelectorOperatorLessOrEqual,

  // [tag>=value]
  SelectorOperatorGreaterOrEqual,

  // [tag=value]
  SelectorOperatorEqual,

  // [tag<value]
  SelectorOperatorLess,

  // [tag>value]
  SelectorOperatorGreater,

  // [!tag]
  SelectorOperatorIsNotSet,

  // [tag]
  SelectorOperatorIsSet,
};

struct SelectorExpression
{
  SelectorOperatorType m_operator;
  std::string m_tag;
  std::string m_value;

  SelectorExpression() : m_operator(SelectorOperatorUnknown) {}
};

bool ParseSelector(std::string const & str, SelectorExpression & e);

}  // namespace drule
