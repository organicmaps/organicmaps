#include "generator/regions/specs/united_kingdom.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel UnitedKingdomSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // England, Scotland, Wales and Northern Ireland
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
