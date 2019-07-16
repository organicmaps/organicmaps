#include "generator/regions/specs/south_sudan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SouthSudanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

