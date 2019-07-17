#include "generator/regions/specs/luxembourg.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel LuxembourgSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     //	Districts (state)
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Cantons (region)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
