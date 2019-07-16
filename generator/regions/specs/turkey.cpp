#include "generator/regions/specs/turkey.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TurkeySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

