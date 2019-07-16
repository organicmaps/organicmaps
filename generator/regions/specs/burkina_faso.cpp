#include "generator/regions/specs/burkina_faso.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel BurkinaFasoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
