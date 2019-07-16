#include "generator/regions/specs/cote_divoire.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel CoteDivoireSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator

