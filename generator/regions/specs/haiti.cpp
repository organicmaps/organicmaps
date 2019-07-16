#include "generator/regions/specs/haiti.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel HaitiSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

