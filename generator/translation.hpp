#pragma once

#include "3party/jansson/myjansson.hpp"
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

/// This function will take the following steps:
/// 1. Return the |languageCode| name if it exists.
/// 2. Return transliteration trying to use kPreferredLanguagesForTransliterate
/// first, then any, if it succeeds.
/// 3. Otherwise, return empty string.
std::string GetTranslatedOrTransliteratedName(StringUtf8Multilang const & name,
                                              LanguageCode languageCode);

class Localizator
{
public:
  struct LabelAndTranslition
  {
    std::string m_label;
    std::string m_translation;
  };

  template <class Fn>
  void AddLocale(Fn && translator)
  {
    auto const & languages = LocaleLanguages();
    for (auto const & language : languages)
    {
      m_localesWithLanguages.emplace_back(LocaleWithLanguage{base::NewJSONObject(), language});
      std::string label;
      std::string translation;
      LabelAndTranslition labelAndTranslation{translator(language)};
      ToJSONObject(*m_localesWithLanguages.back().m_locale, labelAndTranslation.m_label,
                   labelAndTranslation.m_translation);
    }
  }

  base::JSONPtr BuildLocales()
  {
    auto locales = base::NewJSONObject();
    for (auto & localeWithLanguage : m_localesWithLanguages)
      ToJSONObject(*locales, localeWithLanguage.m_language, localeWithLanguage.m_locale);

    m_localesWithLanguages.clear();
    return locales;
  }

private:
  struct LocaleWithLanguage
  {
    base::JSONPtr m_locale;
    std::string m_language;
  };
  using LocalesWithLanguages = std::vector<LocaleWithLanguage>;

  std::vector<std::string> const & LocaleLanguages() const;

  LocalesWithLanguages m_localesWithLanguages;
};
}  // namespace generator