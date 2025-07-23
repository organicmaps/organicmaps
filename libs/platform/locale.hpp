#pragma once

#include <string>

namespace platform
{
std::string const kNonBreakingSpace = "\u00A0";
std::string const kNarrowNonBreakingSpace = "\u202F";

struct Locale
{
public:
  std::string m_language;
  std::string m_country;
  std::string m_currency;
  std::string m_decimalSeparator;
  std::string m_groupingSeparator;
};

Locale GetCurrentLocale();
bool GetLocale(std::string localeName, Locale & result);
}  // namespace platform
