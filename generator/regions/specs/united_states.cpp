#include "generator/regions/specs/united_states.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel UnitedStatesSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // the 50 states, three Territories, two Commonwealths and the
                                // District of Columbia
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // state counties and "county equivalents," territorial
                                   // municipalities
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
