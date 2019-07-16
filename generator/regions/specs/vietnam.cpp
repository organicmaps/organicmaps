#include "generator/regions/specs/vietnam.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel VietnamSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

