#include "generator/regions/specs/tunisia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TunisiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Borders of the 24 Governorates of Tunisia
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Imadats (Sectors) Borders
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
