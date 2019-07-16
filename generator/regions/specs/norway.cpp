#include "generator/regions/specs/norway.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NorwaySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

