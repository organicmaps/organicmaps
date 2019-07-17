#include "generator/regions/specs/denmark.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel DenmarkSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // Regioner (regions - administrative unit)
  case AdminLevel::Seven: return PlaceLevel::Subregion;  // Districts
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
