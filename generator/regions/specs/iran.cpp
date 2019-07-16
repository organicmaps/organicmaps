#include "generator/regions/specs/iran.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IranSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

