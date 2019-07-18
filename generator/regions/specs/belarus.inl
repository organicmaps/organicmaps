#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/country_specifier_builder.hpp"
#include "generator/regions/region.hpp"

#include <string>
#include <vector>

namespace generator
{
namespace regions
{
namespace specs
{
class BelarusSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Belarus"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(BelarusSpecifier);

PlaceLevel BelarusSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Oblasts (вобласьць / область)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Regions (раён / район)
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Soviets of settlement (сельсавет / cельсовет)
  case AdminLevel::Nine: return PlaceLevel::Sublocality;  // Suburbs (раён гораду / район города)
  case AdminLevel::Ten:
    return PlaceLevel::Locality;  // Municipalities (населены пункт / населённый пункт)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
