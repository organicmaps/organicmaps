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
class SierraLeoneSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Sierra Leone"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(SierraLeoneSpecifier);

PlaceLevel SierraLeoneSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Five:
    return PlaceLevel::Region;  // borders of 12 administrative districts, 6 municipalities
                                // (including Freetown), and Western Area Rural
  case AdminLevel::Six:
    return PlaceLevel::Subregion;  // borders of Paramount Chiefdoms, 8 Wards in Freetown, and 4
                                   // Districts in Western Area
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
