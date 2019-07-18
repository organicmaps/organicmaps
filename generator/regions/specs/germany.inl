#include "generator/place_node.hpp"

#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/region.hpp"
#include "generator/regions/country_specifier_builder.hpp"

#include <vector>
#include <string>

namespace generator
{
namespace regions
{
namespace specs
{

class GermanySpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Germany"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(GermanySpecifier);

PlaceLevel GermanySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;        // federal states border
  case AdminLevel::Five: return PlaceLevel::Subregion;     // state-district border
  case AdminLevel::Eight: return PlaceLevel::Sublocality;  // Towns, Municipalities / City-districts
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
