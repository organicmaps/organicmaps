#include "generator/regions/specs/poland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel PolandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // województwa (voivodships, provinces, regions). Details in Polish
  case AdminLevel::Six: return PlaceLevel::Subregion;   // powiaty (counties)
  case AdminLevel::Eight: return PlaceLevel::Locality;  //  cities, towns and villages.
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
