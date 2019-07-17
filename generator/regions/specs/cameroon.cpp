#include "generator/regions/specs/cameroon.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CameroonSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    //  Regions, like Extrême-Nord
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Départements, like Mayo-Danay
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
