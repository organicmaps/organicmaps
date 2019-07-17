#include "generator/regions/specs/liberia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel LiberiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    //	Counties (15)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Districts (90)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
