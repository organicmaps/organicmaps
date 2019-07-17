#include "generator/regions/specs/brunei.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(BruneiSpecifier);

PlaceLevel BruneiSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Districts
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Mukims (subdistricts)
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Villages (kampung or kampong), as designated by the Survey
                                  // Department
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
