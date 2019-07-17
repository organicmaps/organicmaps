#include "generator/regions/specs/french_polynesia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel FrenchPolynesiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six:
    return PlaceLevel::Region;  // Administratrive divisions (archipelagos or sub-archipelagos)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
