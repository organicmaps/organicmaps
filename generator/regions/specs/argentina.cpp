#include "generator/regions/specs/argentina.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(ArgentinaSpecifier);

PlaceLevel ArgentinaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Province / Ciudad Autónoma de Buenos Aires
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Departamento / partido
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Villages, City, town
  case AdminLevel::Nine: return PlaceLevel::Suburb;     // Barrio
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
