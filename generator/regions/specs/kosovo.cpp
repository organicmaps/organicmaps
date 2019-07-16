#include "generator/regions/specs/kosovo.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel KosovoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
