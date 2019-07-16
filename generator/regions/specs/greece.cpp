#include "generator/regions/specs/greece.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GreeceSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
