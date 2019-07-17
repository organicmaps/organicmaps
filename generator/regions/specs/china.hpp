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
class ChinaSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"China"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};
}  // namespace specs
}  // namespace regions
}  // namespace generator
