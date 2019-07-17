#include "generator/regions/specs/andorra.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AndorraSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Seven: return PlaceLevel::Region;  // Parròquies borders (communes)
  case AdminLevel::Eight:
    return PlaceLevel::Sublocality;  // Municipality borders Only used for the capital city Andorra
                                     // la Vella.
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
