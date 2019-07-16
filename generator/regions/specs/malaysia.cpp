#include "generator/regions/specs/malaysia.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MalaysiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
