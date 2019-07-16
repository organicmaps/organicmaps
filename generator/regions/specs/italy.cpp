#include "generator/regions/specs/italy.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel ItalySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
