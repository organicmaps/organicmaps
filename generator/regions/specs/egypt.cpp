#include "generator/regions/specs/egypt.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel EgyptSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three: return PlaceLevel::Region;  // Governorate (Mouhafazah محافظة)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
