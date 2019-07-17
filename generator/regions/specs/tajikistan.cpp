#include "generator/regions/specs/tajikistan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TajikistanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // Border of Province (3), Region (1) and Capital city (Sughd
                                // Province, Region of Republican Subordination, Khatlon Province,
                                // Gorno-Badakhshan Autonomous Region, Dushanbe)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // District (e.g. Varzob, Khovaling, Murghob)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
