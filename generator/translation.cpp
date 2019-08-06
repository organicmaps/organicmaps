#include "translation.hpp"

#include "base/string_utils.hpp"

#include "coding/transliteration.hpp"

#include "platform/platform.hpp"

#include <unordered_map>

namespace
{
// Languages in order for better transliterations. This is kind
// of workaround before real made translations.
using Languages = std::vector<std::string>;

const std::unordered_map<generator::LanguageCode, Languages> kPreferredLanguagesForTransliterate = {
    {StringUtf8Multilang::GetLangIndex("ru"), {"ru", "uk", "be"}},
    {StringUtf8Multilang::GetLangIndex("en"), {"en", "da", "es", "fr"}}};

Languages kLocalelanguages = {"en", "ru"};
}  // namespace

namespace
{
struct TransliterationInitilizer
{
  TransliterationInitilizer()
  {
    Transliteration::Instance().Init(GetPlatform().ResourcesDir());
  }
};
} // namespace
namespace generator
{
std::string GetTranslatedOrTransliteratedName(StringUtf8Multilang const & name,
                                              LanguageCode languageCode)
{
  static TransliterationInitilizer littleStaticHelperToPreventMisinitilizedTranslations;

  std::string s = GetName(name, languageCode);
  if (!s.empty())
    return s;

  if (languageCode != StringUtf8Multilang::kEnglishCode)
    return std::string();

  s = GetName(name, StringUtf8Multilang::kInternationalCode);
  if (!s.empty() && strings::IsASCIIString(s))
    return s;

  s = GetName(name, StringUtf8Multilang::kDefaultCode);
  if (!s.empty() && strings::IsASCIIString(s))
    return s;

  auto const fn = [&s](int8_t code, std::string const & name) {
    if (strings::IsASCIIString(name))
    {
      s = name;
      return base::ControlFlow::Break;
    }

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

  return std::string();
}

Languages const & Localizator::LocaleLanguages() const { return kLocalelanguages; }
}  // namespace generator
