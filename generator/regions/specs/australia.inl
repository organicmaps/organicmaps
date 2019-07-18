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
class AustraliaSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Australia"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(AustraliaSpecifier);

PlaceLevel AustraliaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // State or Territory Border
  case AdminLevel::Seven:
    return PlaceLevel::Subregion;  // District or Region Border (e.g Perthshire, Fitzroy, Canning,
                                   // Greater Sydney, Greater Melbourne, etc.)
  case AdminLevel::Nine:
    return PlaceLevel::Locality;  // Suburb and Locality Border (If larger than ABS boundary)
  case AdminLevel::Ten: return PlaceLevel::Locality;  // Suburb and Locality Border (ABS boundaries)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
