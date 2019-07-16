#include "generator/regions/specs/malawi.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MalawiSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

