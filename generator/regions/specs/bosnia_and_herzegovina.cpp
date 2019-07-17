#include "generator/regions/specs/bosnia_and_herzegovina.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BosniaAndHerzegovinaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // entitet / eнтитет / entity
  case AdminLevel::Five: return PlaceLevel::Subregion;  // kanton (FBiH)
  case AdminLevel::Six: return PlaceLevel::Locality;    // grad / град / city
  case AdminLevel::Nine: return PlaceLevel::Locality;   // naselje / насеље / localities
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
