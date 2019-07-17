#include "generator/regions/specs/gabon.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GabonSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Provinces
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Departments
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
