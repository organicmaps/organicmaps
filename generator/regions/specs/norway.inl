#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/country_specifier_builder.hpp"
#include "generator/regions/region.hpp"

#include <string>
#include <vector>

namespace generator
{
namespace regions
{
namespace specs
{
class NorwaySpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Norway"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(NorwaySpecifier);

PlaceLevel NorwaySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // County
  case AdminLevel::Seven: return PlaceLevel::Subregion;  // Muncipiality
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
