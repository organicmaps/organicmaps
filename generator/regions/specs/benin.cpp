#include "generator/regions/specs/benin.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BeninSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

