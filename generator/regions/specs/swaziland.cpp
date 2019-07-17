#include "generator/regions/specs/swaziland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SwazilandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Tifundza (regions)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Tinkhundla (constituencies)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
