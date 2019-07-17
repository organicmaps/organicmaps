#include "generator/regions/specs/iraq.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IraqSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // governorates
  case AdminLevel::Six: return PlaceLevel::Subregion;  // districts (qadha)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
