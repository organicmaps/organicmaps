#include "generator/regions/specs/cote_divoire.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(CoteDivoireSpecifier);

PlaceLevel CoteDivoireSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // district (example: Lagunes) où disctrict autonome (example:
                                // Abidjan) place=state (not place=district)
  case AdminLevel::Five:
    return PlaceLevel::Subregion;  // region (example: Agnéby-Tiassa) place=region
  case AdminLevel::Nine: return PlaceLevel::Locality;    // village
  case AdminLevel::Ten: return PlaceLevel::Sublocality;  // quartier
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
