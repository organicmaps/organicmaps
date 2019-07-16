#include "generator/regions/specs/czech_republic.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CzechRepublicSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

