#include "generator/regions/specs/israel.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IsraelSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // District
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Sub-district
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
