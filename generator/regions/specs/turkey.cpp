#include "generator/regions/specs/turkey.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TurkeySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // The Census-defined geographical regions of Turkey (which are used
                                // for administrative purposes and are well-known in the country)
  case AdminLevel::Four: return PlaceLevel::Subregion;  // Borders of the 81 provinces of Turkey
  case AdminLevel::Eight: return PlaceLevel::Locality;  //(inofficially: boundaries of villages)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
