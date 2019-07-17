#include "generator/regions/specs/austria.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AustriaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Bundesland
  case AdminLevel::Nine:
    return PlaceLevel::Locality;  // Wiener / Grazer / ... Stadt- / Gemeindebezirk
  case AdminLevel::Ten: return PlaceLevel::Sublocality;  // Stadtteile
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
