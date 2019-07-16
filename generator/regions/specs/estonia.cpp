#include "generator/regions/specs/estonia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel EstoniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

