#include "generator/regions/specs/bangladesh.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BangladeshSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three: return PlaceLevel::Region;    // Division
  case AdminLevel::Five: return PlaceLevel::Subregion;  // District
  case AdminLevel::Nine: return PlaceLevel::Locality;   // Union
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
