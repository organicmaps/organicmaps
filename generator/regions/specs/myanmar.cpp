#include "generator/regions/specs/myanmar.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MyanmarSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
