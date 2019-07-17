#include "generator/regions/specs/russia.hpp"

#include "generator/regions/country_specifier_builder.hpp"

#include "base/stl_helpers.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
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

REGISTER_COUNTRY_SPECIFIER(RussiaSpecifier);

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
