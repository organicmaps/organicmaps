#include "generator/regions/specs/germany.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GermanySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;        // federal states border
  case AdminLevel::Five: return PlaceLevel::Subregion;     // state-district border
  case AdminLevel::Eight: return PlaceLevel::Sublocality;  // Towns, Municipalities / City-districts
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
