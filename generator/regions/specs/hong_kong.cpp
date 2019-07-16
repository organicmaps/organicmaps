#include "generator/regions/specs/hong_kong.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel HongKongSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

