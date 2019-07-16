#include "generator/regions/specs/serbia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SerbiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
