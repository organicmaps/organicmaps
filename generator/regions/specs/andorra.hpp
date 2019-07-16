#include "generator/place_node.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/region.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
class AndorraSpecifier final : public CountrySpecifier
{
private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};
}  // namespace specs
}  // namespace regions
}  // namespace generator
