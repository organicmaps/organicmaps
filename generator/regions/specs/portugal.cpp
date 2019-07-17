#include "generator/regions/specs/portugal.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel PortugalSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Region
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Island / Subregion
  case AdminLevel::Nine: return PlaceLevel::Locality;   // Locality
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
