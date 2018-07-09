#include "search/search_quality/assessment_tool/helpers.hpp"

#include "base/string_utils.hpp"

QString ToQString(strings::UniString const & s)
{
  return QString::fromUtf8(strings::ToUtf8(s).c_str());
}

QString ToQString(std::string const & s)
{
  return QString::fromUtf8(s.c_str());
}
