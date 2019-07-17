#include "generator/regions/specs/cuba.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CubaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Provincia
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Municipio
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
