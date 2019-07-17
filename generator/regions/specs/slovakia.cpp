#include "generator/regions/specs/slovakia.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(SlovakiaSpecifier);

PlaceLevel SlovakiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // region borders
  case AdminLevel::Nine:
    return PlaceLevel::Locality;  // (Town/Village), autonomous towns in Bratislava and Košice
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
