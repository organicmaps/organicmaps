#include "generator/regions/specs/sweden.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SwedenSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

