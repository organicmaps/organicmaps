#include "generator/regions/specs/finland.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel FinlandSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
