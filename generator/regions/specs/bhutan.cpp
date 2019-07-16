#include "generator/regions/specs/bhutan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BhutanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
