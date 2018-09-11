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

  auto const fn = [&s](int8_t code, std::string const & name)
  {
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

StringUtf8Multilang const & RegionWithName::GetStringUtf8MultilangName() const
{
  return m_name;
}

void RegionWithName::SetStringUtf8MultilangName(StringUtf8Multilang const & name)
{
  m_name = name;
}

base::GeoObjectId RegionWithData::GetId() const
{
  return m_regionData.GetOsmId();
}

bool RegionWithData::HasAdminCenter() const
{
  return m_regionData.HasAdminCenter();
}

base::GeoObjectId RegionWithData::GetAdminCenterId() const
{
  return m_regionData.GetAdminCenter();
}

bool RegionWithData::HasIsoCode() const
{
  return m_regionData.HasIsoCodeAlpha2();
}

std::string RegionWithData::GetIsoCode() const
{
  return m_regionData.GetIsoCodeAlpha2();
}

// The values ​​of the administrative level and place are indirectly dependent.
// This is used when calculating the rank.
uint8_t RegionWithData::GetRank() const
{
  auto const adminLevel = GetAdminLevel();
  auto const placeType = GetPlaceType();

  switch (placeType)
  {
  case PlaceType::City:
  case PlaceType::Town:
  case PlaceType::Village:
  case PlaceType::Hamlet:
  case PlaceType::Suburb:
  case PlaceType::Neighbourhood:
  case PlaceType::Locality:
  case PlaceType::IsolatedDwelling: return static_cast<uint8_t>(placeType);
  default: break;
  }

  switch (adminLevel)
  {
  case AdminLevel::Two:
  case AdminLevel::Four:
  case AdminLevel::Six: return static_cast<uint8_t>(adminLevel);
  default: break;
  }

  return kNoRank;
}

std::string RegionWithData::GetLabel() const
{
  auto const adminLevel = GetAdminLevel();
  auto const placeType = GetPlaceType();

  switch (placeType)
  {
  case PlaceType::City:
  case PlaceType::Town:
  case PlaceType::Village:
  case PlaceType::Hamlet: return "locality";
  case PlaceType::Suburb:
  case PlaceType::Neighbourhood: return "suburb";
  case PlaceType::Locality:
  case PlaceType::IsolatedDwelling: return "sublocality";
  default: break;
  }

  switch (adminLevel)
  {
  case AdminLevel::Two: return "country";
  case AdminLevel::Four: return "region";
  case AdminLevel::Six: return "subregion";
  default: break;
  }

  return "";
}
}  // namespace regions
}  // namespace generator
