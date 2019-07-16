#include "generator/regions/specs/burundi.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BurundiSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

