#include "generator/regions/specs/moldova.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MoldovaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Autonomous regions, Districts
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Towns, Villages that do not form a commune
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
