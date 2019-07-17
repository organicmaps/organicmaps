#include "generator/regions/specs/north_korea.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NorthKoreaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Province
  case AdminLevel::Six: return PlaceLevel::Subregion;  // County
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Town (읍), Village (리), Neighbourhood (동), Workers' district
                                  // (로동자구)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
