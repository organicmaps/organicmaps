#include "generator/regions/specs/chad.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(ChadSpecifier);

PlaceLevel ChadSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Régions (23)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Départments (61)
  case AdminLevel::Nine: return PlaceLevel::Locality;  // Villages ou localités
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
