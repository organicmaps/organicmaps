#include "generator/regions/specs/nigeria.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NigeriaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // States (36) + the federal capital territory
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Local Government Areas (774)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
