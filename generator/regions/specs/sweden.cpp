#include "generator/regions/specs/sweden.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
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
