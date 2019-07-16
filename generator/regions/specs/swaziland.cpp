#include "generator/regions/specs/swaziland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SwazilandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
