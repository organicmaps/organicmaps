#include "generator/regions/specs/gambia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GambiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Regions (Divisions)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Districts
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
