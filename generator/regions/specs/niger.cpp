#include "generator/regions/specs/niger.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NigerSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Régions
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Départements
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
