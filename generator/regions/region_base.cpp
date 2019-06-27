#include "generator/regions/region_base.hpp"

#include "coding/transliteration.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"
#include "base/string_utils.hpp"

namespace generator
{
namespace regions
{
namespace
{
// Languages in order for better transliterations. This is kind
// of workaround before real made translations.
const std::unordered_map<RegionWithName::LanguageCode, RegionWithName::PreferredLanguages>
    kPreferredLanguagesForTransliterate = {
        {StringUtf8Multilang::GetLangIndex("ru"), {"en" /*English*/, "ru" /*Русский*/}},
        {StringUtf8Multilang::GetLangIndex("en"), {"en" /*English*/, "ru" /*Русский*/}}};

}  // namespace

std::string RegionWithName::GetName(int8_t lang) const
{
  std::string s;
  VERIFY(m_name.GetString(lang, s) != s.empty(), ());
  return s;
}

std::string RegionWithName::GetTranslatedOrTransliteratedName(
    RegionWithName::LanguageCode languageCode) const
{
  std::string s = GetName(languageCode);
  if (!s.empty() && strings::IsASCIIString(s))
    return s;

  s = GetName(StringUtf8Multilang::kInternationalCode);
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
      m_name.ForEachLanguage(kPreferredLanguagesForTransliterate.at(languageCode), fn))
    m_name.ForEach(fn);

  return s;
}

std::string RegionWithName::GetEnglishOrTransliteratedName() const
{
  return GetTranslatedOrTransliteratedName(StringUtf8Multilang::GetLangIndex("en"));
}

StringUtf8Multilang const & RegionWithName::GetMultilangName() const { return m_name; }

void RegionWithName::SetMultilangName(StringUtf8Multilang const & name) { m_name = name; }

base::GeoObjectId RegionWithData::GetId() const { return m_regionData.GetOsmId(); }

boost::optional<std::string> RegionWithData::GetIsoCode() const
{
  return m_regionData.GetIsoCodeAlpha2();
}

boost::optional<base::GeoObjectId> RegionWithData::GetLabelOsmId() const
{
  return m_regionData.GetLabelOsmId();
}
}  // namespace regions
}  // namespace generator
