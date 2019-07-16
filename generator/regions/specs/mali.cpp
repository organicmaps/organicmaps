#include "generator/regions/specs/mali.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MaliSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

