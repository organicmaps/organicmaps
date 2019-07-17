#include "generator/regions/specs/south_sudan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SouthSudanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // State (28)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // County (86)
  case AdminLevel::Nine: return PlaceLevel::Locality;  // Village
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
