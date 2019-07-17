#include "generator/regions/specs/malawi.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MalawiSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three: return PlaceLevel::Region;    //	Regions
  case AdminLevel::Four: return PlaceLevel::Subregion;  // Districts
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
