#include "generator/regions/specs/bahrain.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BahrainSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Governorate
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Municipality Example: Al Manamah, Ras Rumman
  case AdminLevel::Nine: return PlaceLevel::Locality;  // neighbourhood
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
