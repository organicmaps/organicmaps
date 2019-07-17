#include "generator/regions/specs/romania.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel RomaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // Historical provinces (Transylvania, Moldavia ...)
  case AdminLevel::Four:
    return PlaceLevel::Subregion;  // Counties (Judeţe) and the Municipality of Bucharest
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
