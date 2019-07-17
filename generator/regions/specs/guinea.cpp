#include "generator/regions/specs/guinea.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(GuineaSpecifier);

PlaceLevel GuineaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Region
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Prefectures
  case AdminLevel::Nine: return PlaceLevel::Locality;  // Villages / Towns
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
