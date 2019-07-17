#include "generator/regions/specs/lithuania.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(LithuaniaSpecifier);

PlaceLevel LithuaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     //	Counties (Apskritys)
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Municipalities (Savivaldybės / Rajonai)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
