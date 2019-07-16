#include "generator/regions/specs/japan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel JapanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
