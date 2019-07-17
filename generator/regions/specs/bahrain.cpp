#include "generator/regions/specs/bahrain.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BahrainSpecifier);

PlaceLevel BahrainSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Governorate
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // Municipality Example: Al Manamah, Ras Rumman
  case AdminLevel::Nine: return PlaceLevel::Locality;  // neighbourhood
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
