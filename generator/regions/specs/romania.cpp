#include "generator/regions/specs/romania.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel RomaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
