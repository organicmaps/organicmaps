#include "generator/regions/specs/iceland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IcelandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
