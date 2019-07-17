#include "generator/regions/specs/ukraine.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(UkraineSpecifier);

PlaceLevel UkraineSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Oblasts
  case AdminLevel::Six: return PlaceLevel::Subregion;  // районы в областях
  case AdminLevel::Seven: return PlaceLevel::Sublocality;  // Административные районы в городах
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
