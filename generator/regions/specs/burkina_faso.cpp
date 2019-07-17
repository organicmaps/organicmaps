#include "generator/regions/specs/burkina_faso.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BurkinaFasoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Régions
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Provinces
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Towns or villages (excluding their surrounding rural areas)
  case AdminLevel::Nine:
    return PlaceLevel::Sublocality;  // Urban sectors (only in cities, and in the capital town of
                                     // other urban municipalities)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
