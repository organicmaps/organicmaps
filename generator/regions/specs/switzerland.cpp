#include "generator/regions/specs/switzerland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SwitzerlandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
