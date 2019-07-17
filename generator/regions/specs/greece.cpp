#include "generator/regions/specs/greece.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GreeceSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Five: return PlaceLevel::Region;  // Όρια Περιφερειών (NUTS 2)
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Όρια Περιφερειακών ενοτήτων (NUTS 3)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
