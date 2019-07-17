#include "generator/regions/region_base.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"

namespace generator
{
namespace regions
{
std::string RegionWithName::GetName(LanguageCode languageCode) const
{
  return ::generator::GetName(m_name, languageCode);
}

std::string RegionWithName::GetTranslatedOrTransliteratedName(LanguageCode languageCode) const
{
  return ::generator::GetTranslatedOrTransliteratedName(m_name, languageCode);
}

std::string RegionWithName::GetInternationalName() const
{
  std::string intName = ::generator::GetTranslatedOrTransliteratedName(
      m_name, StringUtf8Multilang::kInternationalCode);

  return intName.empty() ? GetName() : intName;
}

StringUtf8Multilang const & RegionWithName::GetMultilangName() const { return m_name; }

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
