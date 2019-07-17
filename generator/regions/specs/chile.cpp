#include "generator/regions/specs/chile.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel ChileSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Regiones
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Provincias
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
