#include "generator/regions/specs/jordan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel JordanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
