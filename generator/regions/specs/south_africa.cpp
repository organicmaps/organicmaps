#include "generator/regions/specs/south_africa.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SouthAfricaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
