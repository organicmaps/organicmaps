#include "generator/regions/specs/israel.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IsraelSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
