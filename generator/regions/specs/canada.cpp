#include "generator/regions/specs/canada.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CanadaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
