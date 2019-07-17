#include "generator/regions/specs/taiwan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TaiwanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // Province
  case AdminLevel::Seven: return PlaceLevel::Subregion;  // District
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
