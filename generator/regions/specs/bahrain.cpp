#include "generator/regions/specs/bahrain.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BahrainSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
