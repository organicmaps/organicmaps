#include "generator/regions/specs/hungary.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel HungarySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

