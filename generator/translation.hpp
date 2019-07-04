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
/// 1.1 Next steps only for english locale
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
  class EasyObjectWithTranslation
  {
  public:
    explicit EasyObjectWithTranslation(StringUtf8Multilang const & name) : m_name(name) {}
    std::string GetTranslatedOrTransliteratedName(LanguageCode languageCode) const
    {
      return ::generator::GetTranslatedOrTransliteratedName(m_name, languageCode);
    }

    std::string GetName(LanguageCode languageCode = StringUtf8Multilang::kDefaultCode) const
    {
      return ::generator::GetName(m_name, languageCode);
    }

  private:
    StringUtf8Multilang const m_name;
  };

  explicit Localizator(json_t & node) : m_node(GetOrCreateNode("locales", node)) {}

  template <class Object>
  void AddLocale(std::string const & label, Object const & objectWithName,
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
    json_t & locale = GetOrCreateNode(DefaultLocaleName(), m_node);
    json_t & node = GetOrCreateNode(level, locale);
    verboser(node);
  }

private:
  void AddLocale(std::string const & language, std::string const & level, std::string const & name,
                 std::string const & label)
  {
    json_t & locale = GetOrCreateNode(language, m_node);

    if (!level.empty())
    {
      json_t & levelNode = GetOrCreateNode(level, locale);
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

  static json_t & GetOrCreateNode(std::string const & nodeName, json_t & root)
  {
    json_t * node = base::GetJSONOptionalField(&root, nodeName);
    if (!node || base::JSONIsNull(node))
    {
      node = json_object();
      ToJSONObject(root, nodeName, *node);
    }

    return *node;
  }

  std::vector<std::string> const & LocaleLanguages() const;

  json_t & m_node;
};
}  // namespace generator
