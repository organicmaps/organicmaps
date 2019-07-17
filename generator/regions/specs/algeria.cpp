#include "generator/regions/specs/algeria.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(AlgeriaSpecifier);

PlaceLevel AlgeriaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Provinces of Algeria
  case AdminLevel::Six: return PlaceLevel::Subregion;   // Districts of Algeria
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Communes of Algeria, also towns
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
