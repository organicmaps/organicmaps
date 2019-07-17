#include "generator/regions/specs/ireland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IrelandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Five: return PlaceLevel::Region;    // Province
  case AdminLevel::Six: return PlaceLevel::Subregion;  // County
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
