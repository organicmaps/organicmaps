#include "generator/regions/specs/indonesia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IndonesiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
