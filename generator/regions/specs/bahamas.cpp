#include "generator/regions/specs/bahamas.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BahamasSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
