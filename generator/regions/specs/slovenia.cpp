#include "generator/regions/specs/slovenia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SloveniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // reserved for regional borders - if they are ever finalized (SL:
                                // pokrajine)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
