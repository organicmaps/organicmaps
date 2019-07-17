#include "generator/regions/specs/canada.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(CanadaSpecifier);

PlaceLevel CanadaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  //  Provinces & territories
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Regional municipalities & single-tier municipalities
  case AdminLevel::Ten: return PlaceLevel::Locality;  // Neighborhoods
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
