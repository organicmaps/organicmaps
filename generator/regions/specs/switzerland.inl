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
class SwitzerlandSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Switzerland"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(SwitzerlandSpecifier);

PlaceLevel SwitzerlandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Cantons (de: Kantone) Example: Aargau, Vaud
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Districts (de: Bezirke/Ämter, fr: districts)
  case AdminLevel::Nine:
    return PlaceLevel::Suburb;  // Suburbs (de: Stadtkreise/Stadtteile, fr: secteurs)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
