#include "generator/regions/specs/malaysia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MalaysiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    //	negeri (states)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // daerah (districts)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
