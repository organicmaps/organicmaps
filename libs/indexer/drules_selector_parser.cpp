#include "indexer/drules_selector_parser.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace drule
{
namespace
{

bool IsTag(std::string const & str)
{
  // tag consists of a-z or A-Z letters or _ and not empty
  for (auto const c : str)
    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && c != '_')
      return false;
  return !str.empty();
}

}  // namespace

bool ParseSelector(std::string const & str, SelectorExpression & e)
{
  // See https://wiki.openstreetmap.org/wiki/MapCSS/0.2
  // Now we support following expressions
  // [tag!=value]
  // [tag>=value]
  // [tag<=value]
  // [tag=value]
  // [tag>value]
  // [tag<value]
  // [!tag]
  // [tag]

  if (str.empty())
    return false;  // invalid format

  // [!tag]
  if (str[0] == '!')
  {
    std::string tag(str.begin() + 1, str.end());
    if (!IsTag(tag))
      return false;  // invalid format

    e.m_operator = SelectorOperatorIsNotSet;
    e.m_tag = std::move(tag);
    e.m_value.clear();
    return true;
  }

  // [tag]
  if (IsTag(str))
  {
    e.m_operator = SelectorOperatorIsSet;
    e.m_tag = str;
    e.m_value.clear();
    return true;
  }

  // Find first entrance of >, < or =
  size_t pos = std::string::npos;
  size_t len = 0;
  char const c[] = {'>', '<', '=', 0};
  for (size_t i = 0; c[i] != 0; ++i)
  {
    size_t p = str.find(c[i]);
    if (p != std::string::npos)
    {
      pos = (pos == std::string::npos) ? p : std::min(p, pos);
      len = 1;
    }
  }

  // If there is no entrance or no space for tag or value then it is invalid format
  if (pos == 0 || len == 0 || pos == str.size() - 1)
    return false;  // invalid format

  // Dedicate the operator type, real operator position and length
  SelectorOperatorType op = SelectorOperatorUnknown;
  if (str[pos] == '>')
  {
    op = SelectorOperatorGreater;
    if (str[pos + 1] == '=')
    {
      ++len;
      op = SelectorOperatorGreaterOrEqual;
    }
  }
  else if (str[pos] == '<')
  {
    op = SelectorOperatorLess;
    if (str[pos + 1] == '=')
    {
      ++len;
      op = SelectorOperatorLessOrEqual;
    }
  }
  else
  {
    ASSERT(str[pos] == '=', ());
    op = SelectorOperatorEqual;
    if (str[pos - 1] == '!')
    {
      --pos;
      ++len;
      op = SelectorOperatorNotEqual;
    }
  }

  std::string tag(str.begin(), str.begin() + pos);
  if (!IsTag(tag))
    return false;  // invalid format

  e.m_operator = op;
  e.m_tag = std::move(tag);
  e.m_value = std::string(str.begin() + pos + len, str.end());
  return true;
}

}  // namespace drule
