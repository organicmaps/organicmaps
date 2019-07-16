#include "generator/regions/specs/india.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel IndiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

