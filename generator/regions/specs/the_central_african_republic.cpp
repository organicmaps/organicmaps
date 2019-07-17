#include "generator/regions/specs/the_central_african_republic.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(TheCentralAfricanRepublicSpecifier);

PlaceLevel TheCentralAfricanRepublicSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Préfectures, like Ouham
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Sous-préfectures, like Bouca
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
