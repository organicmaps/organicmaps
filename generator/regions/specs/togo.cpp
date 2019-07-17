#include "generator/regions/specs/togo.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TogoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // Région / Commune de Lomé (statut particulier)
  case AdminLevel::Five:
    return PlaceLevel::Subregion;  // Préfecture (note : la préfecture du Golfe, dans la région
                                   // Maritime, couvre également Lomé)
  case AdminLevel::Eleven: return PlaceLevel::Locality;  // Village
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
