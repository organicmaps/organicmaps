#include "generator/place_node.hpp"
#include "generator/regions/country_specifier_builder.hpp"

#include "base/stl_helpers.hpp"

#include <string>
#include <vector>

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
    return {"Russia", "Russian Federation", u8"Россия", u8"Российская Федерация", u8"РФ"};
  }

  // CountrySpecifier overrides:
  void AdjustRegionsLevel(Node::PtrList & outers) override;

  int RelateByWeight(LevelRegion const & lhs, LevelRegion const & rhs) const override;
  static bool IsFederalCity(LevelRegion const & region);

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

REGISTER_COUNTRY_SPECIFIER(RussiaSpecifier);

void RussiaSpecifier::AdjustRegionsLevel(Node::PtrList & outers)
{
  AdjustMoscowAdministrativeDivisions(outers);
}

void RussiaSpecifier::AdjustMoscowAdministrativeDivisions(Node::PtrList & outers)
{
  for (auto const & tree : outers)
    AdjustMoscowAdministrativeDivisions(tree);

  if (!m_moscowRegionWasProcessed)
    LOG(LWARNING, ("Failed to find Moscow region"));
  if (!m_moscowCityWasProcessed)
    LOG(LWARNING, ("Failed to find Moscow city"));
}

void RussiaSpecifier::AdjustMoscowAdministrativeDivisions(Node::Ptr const & tree)
{
  auto const & region = tree->GetData();
  if (region.GetAdminLevel() == AdminLevel::Four && region.GetName() == u8"Москва")
  {
    for (auto & subtree : tree->GetChildren())
    {
      MarkMoscowSubregionsByAdministrativeOkrugs(subtree);
      AdjustMoscowCitySuburbs(subtree);
    }
    return;
  }

  if (region.GetAdminLevel() > AdminLevel::Four)
    return;

  for (auto & subtree : tree->GetChildren())
    AdjustMoscowAdministrativeDivisions(subtree);
}

void RussiaSpecifier::MarkMoscowSubregionsByAdministrativeOkrugs(Node::Ptr & tree)
{
  auto & region = tree->GetData();
  auto const adminLevel = region.GetAdminLevel();
  if (adminLevel == AdminLevel::Five)
  {
    region.SetLevel(PlaceLevel::Subregion);
    m_moscowRegionWasProcessed = true;
    return;
  }

  if (adminLevel > AdminLevel::Five)
    return;

  for (auto & subtree : tree->GetChildren())
    MarkMoscowSubregionsByAdministrativeOkrugs(subtree);
}

void RussiaSpecifier::AdjustMoscowCitySuburbs(Node::Ptr const & tree)
{
  auto & region = tree->GetData();
  if (region.GetPlaceType() == PlaceType::City && region.GetName() == u8"Москва")
  {
    for (auto & subtree : tree->GetChildren())
      MarkMoscowSuburbsByAdministrativeDistricts(subtree);
    return;
  }

  if (region.GetLevel() >= PlaceLevel::Locality)
    return;

  for (auto & subtree : tree->GetChildren())
    AdjustMoscowCitySuburbs(subtree);
}

void RussiaSpecifier::MarkMoscowSuburbsByAdministrativeDistricts(Node::Ptr & tree)
{
  auto & region = tree->GetData();
  if (AdminLevel::Eight == region.GetAdminLevel())
  {
    MarkMoscowAdministrativeDistrict(tree);
    return;
  }

  if (PlaceLevel::Suburb == region.GetLevel())
    region.SetLevel(PlaceLevel::Sublocality);

  for (auto & subtree : tree->GetChildren())
    MarkMoscowSuburbsByAdministrativeDistricts(subtree);
}

void RussiaSpecifier::MarkMoscowAdministrativeDistrict(Node::Ptr & node)
{
  auto & region = node->GetData();
  region.SetLevel(PlaceLevel::Suburb);
  m_moscowCityWasProcessed = true;

  for (auto & subtree : node->GetChildren())
    MarkAllSuburbsToSublocalities(subtree);
}

void RussiaSpecifier::MarkAllSuburbsToSublocalities(Node::Ptr & tree)
{
  auto & region = tree->GetData();
  auto const level = region.GetLevel();
  if (level == PlaceLevel::Locality)  // nested locality
    return;

  if (level == PlaceLevel::Suburb)
    region.SetLevel(PlaceLevel::Sublocality);

  for (auto & subtree : tree->GetChildren())
    MarkAllSuburbsToSublocalities(subtree);
}

int RussiaSpecifier::RelateByWeight(LevelRegion const & lhs, LevelRegion const & rhs) const
{
  if (PlaceLevel::Locality == lhs.GetLevel() && IsFederalCity(lhs))
  {
    auto rhsAdminLevel = rhs.GetAdminLevel();
    if (rhsAdminLevel == AdminLevel::Five || rhsAdminLevel == AdminLevel::Eight)
      return 1;
  }

  if (PlaceLevel::Locality == rhs.GetLevel() && IsFederalCity(rhs))
  {
    auto lhsAdminLevel = lhs.GetAdminLevel();
    if (lhsAdminLevel == AdminLevel::Five || lhsAdminLevel == AdminLevel::Eight)
      return -1;
  }

  return CountrySpecifier::RelateByWeight(lhs, rhs);
}

bool RussiaSpecifier::IsFederalCity(LevelRegion const & region)
{
  if (region.GetPlaceType() != PlaceType::City)
    return false;

  auto && name = region.GetName();
  return name == u8"Москва" || name == u8"Санкт-Петербург" || name == u8"Севастополь";
}

PlaceLevel RussiaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Two: return PlaceLevel::Country;
  case AdminLevel::Four: return PlaceLevel::Region;
  case AdminLevel::Six: return PlaceLevel::Subregion;
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
