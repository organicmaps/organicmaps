#include "generator/regions/specs/myanmar.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(MyanmarSpecifier);

PlaceLevel MyanmarSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // states, regions, union territory, self-administered zones and
                                // divisions
  case AdminLevel::Six:
    return PlaceLevel::Locality;  // cities (as the only city in Myanmar, Yangon encompasses several
                                  // districts) ,
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
