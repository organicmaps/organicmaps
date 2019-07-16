#include "generator/regions/specs/new_zealand.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NewZealandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

