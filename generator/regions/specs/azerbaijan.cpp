#include "generator/regions/specs/azerbaijan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AzerbaijanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // autonomous republic
  case AdminLevel::Seven:
    return PlaceLevel::Subregion;  // regions (rayons) and cities with authority of republic level
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
