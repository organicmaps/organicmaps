#include "generator/regions/specs/ethiopia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel EthiopiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Administrative States (9)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Woreda
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
