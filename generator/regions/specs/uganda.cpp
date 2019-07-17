#include "generator/regions/specs/uganda.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel UgandaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Borders of the 112 districts of Uganda
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Borders of the 112 districts of Uganda
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
