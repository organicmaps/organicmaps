#pragma once

#include <string>

namespace platform
{
struct Locale
{
public:
  std::string m_language;
  std::string m_country;
  std::string m_currency;
};

Locale GetCurrentLocale();
}  // namespace platform
