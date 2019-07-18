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
class SwedenSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Sweden"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(SwedenSpecifier);

PlaceLevel SwedenSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // Landsdel (Region) Example: Norrland, Svealand och Götaland
  case AdminLevel::Four:
    return PlaceLevel::Subregion;  // Län (County / NUTS3) (21) Example: Västra Götalands län,
                                   // Örebro län etc
  case AdminLevel::Nine:
    return PlaceLevel::Suburb;  // Suburbs (de: Stadtkreise/Stadtteile, fr: secteurs)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
