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
class BurkinaFasoSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Burkina Faso"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(BurkinaFasoSpecifier);

PlaceLevel BurkinaFasoSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;     // Régions
  case AdminLevel::Five: return PlaceLevel::Subregion;  // Provinces
  case AdminLevel::Eight:
    return PlaceLevel::Locality;  // Towns or villages (excluding their surrounding rural areas)
  case AdminLevel::Nine:
    return PlaceLevel::Sublocality;  // Urban sectors (only in cities, and in the capital town of
                                     // other urban municipalities)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
