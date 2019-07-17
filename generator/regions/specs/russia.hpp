#pragma once

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
class RussiaSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames()
  {
    return {"Russia", u8"Россия", u8"Российская Федерация", u8"РФ"};
  }

  // CountrySpecifier overrides:
  void AdjustRegionsLevel(Node::PtrList & outers) override;

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;

  void AdjustMoscowAdministrativeDivisions(Node::PtrList & outers);
  void AdjustMoscowAdministrativeDivisions(Node::Ptr const & tree);
  void MarkMoscowSubregionsByAdministrativeOkrugs(Node::Ptr & node);

  void AdjustMoscowCitySuburbs(Node::Ptr const & tree);
  void MarkMoscowSuburbsByAdministrativeDistricts(Node::Ptr & tree);
  void MarkMoscowAdministrativeDistrict(Node::Ptr & node);
  void MarkAllSuburbsToSublocalities(Node::Ptr & tree);

  bool m_moscowRegionWasProcessed{false};
  bool m_moscowCityWasProcessed{false};
};
}  // namespace specs
}  // namespace regions
}  // namespace generator
