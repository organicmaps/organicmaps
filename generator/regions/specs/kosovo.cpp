#include "generator/regions/specs/kosovo.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel KosovoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Komunat e Kosovës
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
