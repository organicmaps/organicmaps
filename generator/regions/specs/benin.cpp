#include "generator/regions/specs/benin.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BeninSpecifier);

PlaceLevel BeninSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // Communities, regions and language areas of Belgium
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Provinces
  case AdminLevel::Nine: return PlaceLevel::Suburb;    // Deelgemeenten (sections)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
