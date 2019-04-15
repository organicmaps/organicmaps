#include "generator/regions/region_base.hpp"

#include "coding/transliteration.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"

namespace generator
{
namespace regions
{
std::string RegionWithName::GetName(int8_t lang) const
{
  std::string s;
  VERIFY(m_name.GetString(lang, s) != s.empty(), ());
  return s;
}

std::string RegionWithName::GetEnglishOrTransliteratedName() const
{
  std::string s = GetName(StringUtf8Multilang::kEnglishCode);
  if (!s.empty())
    return s;

  auto const fn = [&s](int8_t code, std::string const & name) {
    if (code != StringUtf8Multilang::kDefaultCode &&
        Transliteration::Instance().Transliterate(name, code, s))
    {
      return base::ControlFlow::Break;
    }

    return base::ControlFlow::Continue;
  };

  m_name.ForEach(fn);
  return s;
}

StringUtf8Multilang const & RegionWithName::GetMultilangName() const
{
  return m_name;
}

void RegionWithName::SetMultilangName(StringUtf8Multilang const & name)
{
  m_name = name;
}

base::GeoObjectId RegionWithData::GetId() const
{
  return m_regionData.GetOsmId();
}

bool RegionWithData::HasIsoCode() const
{
  return m_regionData.HasIsoCodeAlpha2();
}

std::string RegionWithData::GetIsoCode() const
{
  return m_regionData.GetIsoCodeAlpha2();
}
}  // namespace regions
}  // namespace generator
