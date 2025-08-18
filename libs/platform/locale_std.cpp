#include "platform/locale.hpp"

#include <locale>

namespace platform
{
Locale StdLocale2Locale(std::locale loc)
{
  return {"", "", std::use_facet<std::moneypunct<char, true>>(loc).curr_symbol(),
          std::string(1, std::use_facet<std::numpunct<char>>(loc).decimal_point()),
          std::string(1, std::use_facet<std::numpunct<char>>(loc).thousands_sep())};
}

Locale GetCurrentLocale()
{
  // Environment's default locale.
  std::locale loc;

  return StdLocale2Locale(loc);
}

bool GetLocale(std::string const localeName, Locale & result)
{
  try
  {
    std::locale loc(localeName);

    result = StdLocale2Locale(loc);

    return true;
  }
  catch (...)
  {
    return false;
  }
}

}  // namespace platform
