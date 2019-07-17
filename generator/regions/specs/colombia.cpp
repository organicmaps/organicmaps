#include "generator/regions/specs/colombia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel ColombiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Departamento
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Provincia
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
