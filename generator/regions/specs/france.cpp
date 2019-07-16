#include "generator/regions/specs/france.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel FranceSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

