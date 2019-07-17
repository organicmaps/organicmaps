#include "generator/regions/specs/australia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AustraliaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // State or Territory Border
  case AdminLevel::Seven:
    return PlaceLevel::Subregion;  // District or Region Border (e.g Perthshire, Fitzroy, Canning,
                                   // Greater Sydney, Greater Melbourne, etc.)
  case AdminLevel::Nine:
    return PlaceLevel::Locality;  // Suburb and Locality Border (If larger than ABS boundary)
  case AdminLevel::Ten: return PlaceLevel::Locality;  // Suburb and Locality Border (ABS boundaries)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
