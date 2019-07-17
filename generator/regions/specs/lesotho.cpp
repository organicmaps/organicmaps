#include "generator/regions/specs/lesotho.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel LesothoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    //	Districts
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Constitutencies
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
