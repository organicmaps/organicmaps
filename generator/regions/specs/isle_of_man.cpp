#include "generator/regions/specs/isle_of_man.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IsleOfManSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

