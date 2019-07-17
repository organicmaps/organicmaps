#include "generator/regions/specs/haiti.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel HaitiSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // State border (Départements) (Layer 1)
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Districts
  case AdminLevel::Eight: return PlaceLevel::Locality;  // City and town border (communes) (Layer 2)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
