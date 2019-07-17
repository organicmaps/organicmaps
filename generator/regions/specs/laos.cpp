#include "generator/regions/specs/laos.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(LaosSpecifier);

PlaceLevel LaosSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  //	Provinces (Lao: ແຂວງ khoueng) or one prefecture (kampheng
                                // nakhon)
  case AdminLevel::Six: return PlaceLevel::Subregion;   // Districts (Lao: ເມືອງ mɯ́ang)
  case AdminLevel::Seven: return PlaceLevel::Locality;  // Villages
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
