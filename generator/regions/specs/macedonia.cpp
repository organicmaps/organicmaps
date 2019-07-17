#include "generator/regions/specs/macedonia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MacedoniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  //	Статистички Региони (Statistical regions)
  case AdminLevel::Six: return PlaceLevel::Locality;    // Град Скопје (City of Skopje)
  case AdminLevel::Eight: return PlaceLevel::Locality;  // Град/Село (city / town / village)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
