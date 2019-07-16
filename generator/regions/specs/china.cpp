#include "generator/regions/specs/china.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel ChinaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
