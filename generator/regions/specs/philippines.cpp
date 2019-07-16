#include "generator/regions/specs/philippines.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel PhilippinesSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

