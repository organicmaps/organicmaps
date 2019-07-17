#include "generator/regions/specs/isle_of_man.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(IsleOfManSpecifier);

PlaceLevel IsleOfManSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six: return PlaceLevel::Suburb;      // Sheedings
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Parish / Village / Town
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
