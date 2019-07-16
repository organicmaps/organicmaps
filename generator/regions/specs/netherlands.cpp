#include "generator/regions/specs/netherlands.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NetherlandsSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
