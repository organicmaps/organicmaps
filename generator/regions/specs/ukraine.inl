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

class UkraineSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Ukraine"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(UkraineSpecifier);

PlaceLevel UkraineSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Oblasts
  case AdminLevel::Six: return PlaceLevel::Subregion;  // районы в областях
  case AdminLevel::Seven: return PlaceLevel::Sublocality;  // Административные районы в городах
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
