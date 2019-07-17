#include "generator/regions/specs/croatia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CroatiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six: return PlaceLevel::Region;  // County (hr: županije)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
