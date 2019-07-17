#include "generator/regions/specs/philippines.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel PhilippinesSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three: return PlaceLevel::Region;    // Regions (Rehiyon)
  case AdminLevel::Four: return PlaceLevel::Subregion;  // Provinces (Lalawigan)
  case AdminLevel::Six: return PlaceLevel::Locality;    // Cities/municipalities (Lungsod/bayan)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
