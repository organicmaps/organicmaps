#include "generator/regions/specs/nepal.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NepalSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
