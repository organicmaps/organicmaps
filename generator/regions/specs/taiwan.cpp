#include "generator/regions/specs/taiwan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TaiwanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
