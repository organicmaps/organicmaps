#include "generator/regions/specs/angola.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AngolaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Províncias
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Municípios (Municipalities)
  case AdminLevel::Seven:
    return PlaceLevel::Locality;  // Comunas (Urban districts or Communes) includes towns
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
