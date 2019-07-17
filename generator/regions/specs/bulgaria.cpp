#include "generator/regions/specs/bulgaria.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BulgariaSpecifier);

PlaceLevel BulgariaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Райони за планиране
  case AdminLevel::Six: return PlaceLevel::Subregion;   // Regions (Области)
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Borders of city, town, village
  case AdminLevel::Nine: return PlaceLevel::Suburb;     // Districts and suburbs
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
