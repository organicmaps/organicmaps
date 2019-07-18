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
class TurkeySpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Turkey"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(TurkeySpecifier);

PlaceLevel TurkeySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // The Census-defined geographical regions of Turkey (which are used
                                // for administrative purposes and are well-known in the country)
  case AdminLevel::Four: return PlaceLevel::Subregion;  // Borders of the 81 provinces of Turkey
  case AdminLevel::Eight: return PlaceLevel::Locality;  //(inofficially: boundaries of villages)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
