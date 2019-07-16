#include "generator/regions/specs/ukraine.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel UkraineSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

