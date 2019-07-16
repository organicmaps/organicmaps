#include "generator/regions/specs/united_states.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel UnitedStatesSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

