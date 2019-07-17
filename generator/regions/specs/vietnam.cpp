#include "generator/regions/specs/vietnam.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(VietnamSpecifier);

PlaceLevel VietnamSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // province border
  case AdminLevel::Six: return PlaceLevel::Subregion;   // district / township border
  case AdminLevel::Eight: return PlaceLevel::Locality;  // commune / town / ward
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
