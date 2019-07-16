#include "generator/regions/specs/albania.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AlbaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
