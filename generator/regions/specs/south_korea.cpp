#include "generator/regions/specs/south_korea.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SouthKoreaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

