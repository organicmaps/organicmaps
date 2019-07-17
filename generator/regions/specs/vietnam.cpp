#include "generator/regions/specs/vietnam.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel VietnamSpecifier::GetSpecificCountryLevel(Region const & region) const
{

  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // province border
  case AdminLevel::Six: return PlaceLevel::Subregion;   // district / township border
  case AdminLevel::Eight: return PlaceLevel::Locality;  // commune / town / ward
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
