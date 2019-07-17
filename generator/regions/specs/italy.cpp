#include "generator/regions/specs/italy.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(ItalySpecifier);

PlaceLevel ItalySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // per i confini regionali (en: boundary of regions)
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Sub-per i confini provinciali (en: boundary of provinces)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
