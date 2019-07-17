#include "generator/regions/specs/iceland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IcelandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Five: return PlaceLevel::Region;  // Departamentos (State Border)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
