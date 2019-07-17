#include "generator/regions/specs/vanuatu.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(VanuatuSpecifier);

PlaceLevel VanuatuSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Province
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Area (sometimes referred to as Area Council. Municipalities of
                                   // Port Vila and Luganville are their own Areas)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
