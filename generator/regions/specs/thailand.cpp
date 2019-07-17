#include "generator/regions/specs/thailand.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel ThailandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // Province / Bangkok
  case AdminLevel::Six: return PlaceLevel::Subregion;    // District / Bangkok: Khet
  case AdminLevel::Eleven: return PlaceLevel::Locality;  // Village
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
