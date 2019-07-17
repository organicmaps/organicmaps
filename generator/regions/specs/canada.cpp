#include "generator/regions/specs/canada.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CanadaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  //  Provinces & territories
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Regional municipalities & single-tier municipalities
  case AdminLevel::Ten: return PlaceLevel::Locality;  // Neighborhoods
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
