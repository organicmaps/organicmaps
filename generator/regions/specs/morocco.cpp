#include "generator/regions/specs/morocco.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MoroccoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Regions (Wilaya)
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Provinces,
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
