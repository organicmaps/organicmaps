#include "generator/regions/specs/north_korea.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel NorthKoreaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
