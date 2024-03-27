#include "platform/locale.hpp"

#include <string>

using namespace platform;

namespace platform
{
std::string GetSystemDecimalSeparator()
{
  Locale loc = GetCurrentLocale();

  return loc.m_decimalSeparator;
}

std::string GetSystemGroupSeparator()
{
  Locale loc = GetCurrentLocale();

  return loc.m_groupingSeparator;
}

std::string ReplaceGroupingSeparators(std::string const & valueString, std::string const & groupingSeparator)
{
  std::string out(valueString);

  if (groupingSeparator == ",")
    return out;

  size_t pos;

  while((pos = out.find(",")) != std::string::npos)
    out.replace(pos, 1, groupingSeparator);

  return out;
}

std::string ReplaceDecimalSeparator(std::string const & valueString, std::string const & decimalSeparator)
{
  std::string out(valueString);

  if (decimalSeparator == ".")
    return out;

  size_t pos = valueString.find(".");

  if (pos != std::string::npos)
    out.replace(pos, 1, decimalSeparator);

  return out;
}

std::string LocalizeValueString(std::string const & valueString, Locale const & loc)
{
  std::string out;

  if (valueString.find(".") != std::string::npos)
  {
    // String contains a value with decimal separator.
    out = ReplaceDecimalSeparator(valueString, loc.m_decimalSeparator);
  }
  else if (valueString.find(",") != std::string::npos)
  {
    // String contains a value with grouping separator.
    out = ReplaceGroupingSeparators(valueString, loc.m_groupingSeparator);
  }
  else
    out = valueString;

  return out;
}
}
