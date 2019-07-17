#include "generator/regions/specs/finland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel FinlandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Provinces
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Regions
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
