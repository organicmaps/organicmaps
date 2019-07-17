#include "generator/regions/specs/latvia.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(LatviaSpecifier);

PlaceLevel LatviaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Six:
    return PlaceLevel::Region;  //	Counties (Novadi), Cities (Republikas nozīmes pilsētas)
  case AdminLevel::Seven: return PlaceLevel::Locality;  // Towns (Pilsētas)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
