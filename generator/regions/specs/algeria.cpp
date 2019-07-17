#include "generator/regions/specs/algeria.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AlgeriaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Provinces of Algeria
  case AdminLevel::Six: return PlaceLevel::Subregion;   // Districts of Algeria
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Communes of Algeria, also towns
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
