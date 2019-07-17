#include "generator/regions/specs/madagascar.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MadagascarSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three: return PlaceLevel::Region;    //	Faritany mizakatena (provinces)
  case AdminLevel::Four: return PlaceLevel::Subregion;  // Faritra (regions)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
