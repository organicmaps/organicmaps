#include "generator/regions/specs/hungary.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel HungarySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Departamentos (State Border)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Municipios (Municipal Border)
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Aldeas (Admin level border which encompass several towns /
                                  // cities)
  case AdminLevel::Nine: return PlaceLevel::Locality;  // Ciudades (Cities)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
