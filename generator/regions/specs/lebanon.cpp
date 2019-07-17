#include "generator/regions/specs/lebanon.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel LebanonSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three: return PlaceLevel::Region;  //	Governorate (Mouhafazah محافظة)
  case AdminLevel::Four: return PlaceLevel::Subregion;  // Qadaa (also known as Caza قضاء ج. أقضية)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
