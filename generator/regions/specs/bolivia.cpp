#include "generator/regions/specs/bolivia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BoliviaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Departamentos
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Provincias / provinces
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
