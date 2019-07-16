#include "generator/regions/specs/french_polynesia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel FrenchPolynesiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
