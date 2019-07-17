#include "generator/regions/specs/aruba.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel ArubaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Eight:
    return PlaceLevel::Region;  // Districts (Municipalities) known as GAC regions
  default: break;
  }
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
