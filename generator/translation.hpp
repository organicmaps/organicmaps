#pragma once

#include "coding/string_utf8_multilang.hpp"

#include "3party/jansson/myjansson.hpp"

#include <string>
#include <vector>

namespace generator
{
using LanguageCode = int8_t;

inline std::string GetName(StringUtf8Multilang const & name, LanguageCode lang)
{
  std::string s;
  VERIFY(name.GetString(lang, s) != s.empty(), ());
  return s;
}

/// This function will take the following steps:
/// 1. Return the |languageCode| name if it exists.
/// 2. Try to get international name.
/// 3. Try to check if default name is ASCII and return it if succeeds.
/// 4. Return transliteration trying to use kPreferredLanguagesForTransliterate
/// first, then any, if it succeeds.
/// 5. Otherwise, return empty string.
std::string GetTranslatedOrTransliteratedName(StringUtf8Multilang const & name,
                                              LanguageCode languageCode);

class Localizator
{
public:
  template <class Object>
  void AddLocale(std::string const & label, Object objectWithName,
                 std::string const & level = std::string())
  {
    AddLocale(DefaultLocaleName(), level, objectWithName.GetName(), label);

    auto const & languages = LocaleLanguages();
    for (std::string const & language : languages)
    {
      std::string const & translation = objectWithName.GetTranslatedOrTransliteratedName(
          StringUtf8Multilang::GetLangIndex(language));

      if (translation.empty())
        continue;

      AddLocale(language, level, translation, label);
    }
  }

  template <class Verboser>
  void AddVerbose(Verboser && verboser, std::string const & level)
  {
    json_t & locale = GetLocale(DefaultLocaleName());
    json_t & node = GetLevel(level, locale);
    verboser(node);
  }

  base::JSONPtr BuildLocales()
  {
    auto locales = base::NewJSONObject();
    for (auto & localeWithLanguage : m_localesByLanguages)
      ToJSONObject(*locales, localeWithLanguage.first, *localeWithLanguage.second);

    m_localesByLanguages.clear();
    return locales;
  }

private:
  void AddLocale(std::string const & language, std::string const & level, std::string const & name,
                 std::string const & label)
  {
    json_t & locale = GetLocale(language);

    if (!level.empty())
    {
      json_t & levelNode = GetLevel(level, locale);
      ToJSONObject(levelNode, label, name);
    }
    else
    {
      ToJSONObject(locale, label, name);
    }
  }
  static std::string const & DefaultLocaleName()
  {
    static std::string const kDefaultLocaleName = "default";
    return kDefaultLocaleName;
  }

  json_t & GetLocale(std::string const & language)
  {
    if (m_localesByLanguages.find(language) == m_localesByLanguages.end())
      m_localesByLanguages[language] = json_object();

    return *m_localesByLanguages.at(language);
  }

  static json_t & GetLevel(std::string const & level, json_t & locale)
  {
    json_t * levelNode = base::GetJSONOptionalField(&locale, level);

    if (!levelNode)
    {
      levelNode = json_object();
      ToJSONObject(locale, level, *levelNode);
    }

    return *levelNode;
  }
  std::vector<std::string> const & LocaleLanguages() const;

  using LocalesByLanguages = std::map<std::string, json_t *>;
  LocalesByLanguages m_localesByLanguages;
};
}  // namespace generator