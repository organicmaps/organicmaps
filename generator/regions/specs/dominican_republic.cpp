#include "generator/regions/specs/dominican_republic.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel DominicanRepublicSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Province (Provincia)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Municipality (Municipio)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
