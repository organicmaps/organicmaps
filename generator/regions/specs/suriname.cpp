#include "generator/regions/specs/suriname.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SurinameSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // Districten (districts equivalent to counties or provinces)
  case AdminLevel::Eight:
    return PlaceLevel::Suburb;  // Ressorten (resorts approximately equivalent to municipalities)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
