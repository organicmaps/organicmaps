#include "generator/regions/specs/new_zealand.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NewZealandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // Regions (Canterbury, Bay of Plenty, Auckland, Gisborne etc.)
                                // governed by a regional council or unitary authority, and the
                                // Chatham Islands Territory.
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Districts and Cities
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
