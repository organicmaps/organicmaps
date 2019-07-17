#include "generator/regions/specs/cyprus.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CyprusSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six: return PlaceLevel::Region;  // The 6 districts
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
