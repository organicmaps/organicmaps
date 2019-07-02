#include "translation.hpp"

#include "base/string_utils.hpp"

#include "coding/transliteration.hpp"

#include <unordered_map>

namespace
{
// Languages in order for better transliterations. This is kind
// of workaround before real made translations.
using Languages = std::vector<std::string>;

const std::unordered_map<generator::LanguageCode, Languages> kPreferredLanguagesForTransliterate = {
    {StringUtf8Multilang::GetLangIndex("ru"), {"en" /*English*/, "ru" /*Русский*/}},
    {StringUtf8Multilang::GetLangIndex("en"), {"en" /*English*/, "ru" /*Русский*/}}};

Languages kLocalelanguages = {"en", "ru"};
}  // namespace

namespace generator
{
std::string GetTranslatedOrTransliteratedName(StringUtf8Multilang const & name,
                                              LanguageCode languageCode)
{
  std::string s = GetName(name, languageCode);
  if (!s.empty())
    return s;

  s = GetName(name, StringUtf8Multilang::kInternationalCode);
  if (!s.empty() && strings::IsASCIIString(s))
    return s;

  auto const fn = [&s](int8_t code, std::string const & name) {
    if (code != StringUtf8Multilang::kDefaultCode &&
        Transliteration::Instance().Transliterate(name, code, s) && strings::IsASCIIString(s))
    {
      return base::ControlFlow::Break;
    }

    return base::ControlFlow::Continue;
  };

  if (kPreferredLanguagesForTransliterate.count(languageCode) &&
      !name.ForEachLanguage(kPreferredLanguagesForTransliterate.at(languageCode), fn))
    name.ForEach(fn);

  return s;
}

Languages const & Localizator::LocaleLanguages() const { return kLocalelanguages; }
}  // namespace generator
