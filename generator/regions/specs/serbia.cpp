#include "generator/regions/specs/serbia.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(SerbiaSpecifier);

PlaceLevel SerbiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // autonomous provinces аутономне покрајине
  case AdminLevel::Six: return PlaceLevel::Subregion;   // окрузи
  case AdminLevel::Seven: return PlaceLevel::Locality;  // cities градови
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
