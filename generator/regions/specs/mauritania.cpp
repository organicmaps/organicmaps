#include "generator/regions/specs/mauritania.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MauritaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Régions (Wilayas)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Départements (Moughataa)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
