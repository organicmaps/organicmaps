#include "generator/regions/specs/netherlands.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(NetherlandsSpecifier);

PlaceLevel NetherlandsSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // border around The Netherlands, and border around other
                                // constituent states in the Kingdom of the Netherlands (Aruba,
                                // Curaçao & Sint Maarten)
  case AdminLevel::Four:
    return PlaceLevel::Subregion;  // Provinces like Zeeland, Noord-Holland etc. (provincie)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
