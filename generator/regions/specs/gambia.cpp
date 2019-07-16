#include "generator/regions/specs/gambia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GambiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
