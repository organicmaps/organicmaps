#include "generator/regions/specs/iran.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IranSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Province
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Counties
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
