#include "generator/regions/specs/barbados.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BarbadosSpecifier);

PlaceLevel BarbadosSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six: return PlaceLevel::Region;   // Parishes of Barbados
  case AdminLevel::Nine: return PlaceLevel::Suburb;  // Suburbs, Hamlets and Villages
  case AdminLevel::Ten: return PlaceLevel::Suburb;   // Neighbourhoods, Housing Developments
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
