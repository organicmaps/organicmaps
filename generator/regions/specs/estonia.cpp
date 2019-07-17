#include "generator/regions/specs/estonia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel EstoniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six: return PlaceLevel::Region;  // Countys
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
