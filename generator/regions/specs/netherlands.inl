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
class NetherlandsSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Netherlands"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(NetherlandsSpecifier);

PlaceLevel NetherlandsSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // border around The Netherlands, and border around other
                                // constituent states in the Kingdom of the Netherlands (Aruba,
                                // Curaçao & Sint Maarten)
  case AdminLevel::Four:
    return PlaceLevel::Subregion;  // Provinces like Zeeland, Noord-Holland etc. (provincie)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
