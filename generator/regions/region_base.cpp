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
// Languages in order for better transliterations for Russian. This is kind
// of workaround before real made translations.
const std::vector<std::string> kRuPreferredLanguagesForTransliterate = {
    "en" /*English*/,
    "ru" /*Русский*/,
};
}  // namespace

std::string RegionWithName::GetName(int8_t lang) const
{
  std::string s;
  VERIFY(m_name.GetString(lang, s) != s.empty(), ());
  return s;
}

std::string RegionWithName::GetEnglishOrTransliteratedName() const
{
  std::string s = GetName(StringUtf8Multilang::kEnglishCode);
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

  if (!m_name.ForEachLanguage(kRuPreferredLanguagesForTransliterate, fn))
    m_name.ForEach(fn);

  return s;
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
