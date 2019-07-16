#include "generator/regions/specs/united_kingdom.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel UnitedKingdomSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
