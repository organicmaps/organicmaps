#include "generator/regions/specs/senegal.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel SenegalSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
