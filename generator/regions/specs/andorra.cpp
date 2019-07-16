#include "generator/regions/specs/andorra.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AndorraSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
