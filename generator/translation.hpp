#pragma once

#include "coding/string_utf8_multilang.hpp"

namespace generator
{
using LanguageCode = int8_t;

inline std::string GetName(StringUtf8Multilang const & name, int8_t lang)
{
  std::string s;
  VERIFY(name.GetString(lang, s) != s.empty(), ());
  return s;
}

std::string GetTranslatedOrTransliteratedName(StringUtf8Multilang const & name,
                                              LanguageCode languageCode);

}  // namespace generator