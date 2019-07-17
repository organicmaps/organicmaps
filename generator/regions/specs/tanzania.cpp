#include "generator/regions/specs/tanzania.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TanzaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // Region (e.g. Arusha)
  case AdminLevel::Five: return PlaceLevel::Subregion;   // District (e.g. Arumeru)
  case AdminLevel::Eleven: return PlaceLevel::Locality;  // Village
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
