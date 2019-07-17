#include "generator/regions/specs/south_korea.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SouthKoreaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Regional(State)
  case AdminLevel::Seven: return PlaceLevel::Locality;  // City;
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Town;
  case AdminLevel::Ten: return PlaceLevel::Locality;    // Village
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
