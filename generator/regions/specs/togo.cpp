#include "generator/regions/specs/togo.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel TogoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
