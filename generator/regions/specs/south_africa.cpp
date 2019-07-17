#include "generator/regions/specs/south_africa.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SouthAfricaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // provincial borders
  case AdminLevel::Six: return PlaceLevel::Subregion;  // district borders
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
