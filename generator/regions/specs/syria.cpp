#include "generator/regions/specs/syria.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SyriaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Governorates
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Districts
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Cities, towns and villages of Subdistrict
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
