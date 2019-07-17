#include "generator/regions/specs/bhutan.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BhutanSpecifier);

PlaceLevel BhutanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  //  	Dzongkhag (the primary subdivisions of Bhutan)
  case AdminLevel::Five:
    return PlaceLevel::Subregion;  //  	Dungkhag (sub-district of a
                                   //  dzongkhag)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
