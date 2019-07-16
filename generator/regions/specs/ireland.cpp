#include "generator/regions/specs/ireland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IrelandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

