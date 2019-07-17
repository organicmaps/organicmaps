#include "generator/regions/specs/belize.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BelizeSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // Département
  case AdminLevel::Eight: return PlaceLevel::Subregion;  // Commune
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
