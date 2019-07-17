#include "generator/regions/specs/brazil.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BrazilSpecifier);

PlaceLevel BrazilSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // Unidades Federativas (Estados e Distrito Federal)
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Mesorregiões
  case AdminLevel::Ten: return PlaceLevel::Suburb;      // Bairros e Sub-Distritos
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
