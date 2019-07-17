#include "generator/regions/specs/switzerland.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
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
