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
class UnitedStatesSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames()
  {
    return {"United States", "United States of America"};
  }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(UnitedStatesSpecifier);

PlaceLevel UnitedStatesSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four:
    return PlaceLevel::Region;  // the 50 states, three Territories, two Commonwealths and the
                                // District of Columbia
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // state counties and "county equivalents," territorial
                                   // municipalities
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
