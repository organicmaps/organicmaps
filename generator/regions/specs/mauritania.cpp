#include "generator/regions/specs/mauritania.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MauritaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
