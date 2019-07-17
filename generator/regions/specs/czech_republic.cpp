#include "generator/regions/specs/czech_republic.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CzechRepublicSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six: return PlaceLevel::Region;       // Regions
  case AdminLevel::Seven: return PlaceLevel::Subregion;  // Districts
  case AdminLevel::Eight: return PlaceLevel::Locality;   // Towns / village
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
