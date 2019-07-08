#include "generator/regions/locality_point_integrator.hpp"

#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/regions_builder.hpp"

#include "base/string_utils.hpp"

#include <utility>
#include <vector>

#include <boost/geometry.hpp>

namespace generator
{
namespace regions
{
LocalityPointIntegrator::LocalityPointIntegrator(PlacePoint const & localityPoint,
    CountrySpecifier const & countrySpecifier)
  : m_localityRegion{MakeAroundRegion(localityPoint, countrySpecifier)}
  , m_countrySpecifier{countrySpecifier}
{
  CHECK(countrySpecifier.GetLevel(localityPoint.GetPlaceType()) == PlaceLevel::Locality, ());
}

bool LocalityPointIntegrator::IntegrateInto(Node::Ptr & tree)
{
  auto & region = tree->GetData();

  if (HasIntegratingLocalityName(region))
  {
    // Match any locality type (city/town/village).
    if (region.GetLevel() == PlaceLevel::Locality)
    {
      EnlargeRegion(region);
      return true;
    }

    if (IsSuitableForLocalityRegionize(region))
      RegionizeBy(region);
  }

  // Find deepest region for integration.
  for (auto & subtree : tree->GetChildren())
  {
    auto const & subregion = subtree->GetData();
    if (!subregion.Contains(m_localityRegion.GetCenter()))
      continue;

    if (IntegrateInto(subtree))
      return true;
  }

  if (0 < RegionsBuilder::CompareAffiliation(region, m_localityRegion, m_countrySpecifier))
  {
    InsertInto(tree);
    return true;
  }

  return false;
}

bool LocalityPointIntegrator::HasIntegratingLocalityName(LevelRegion const & region) const
{
  auto const & regionName = region.GetName();
  auto const & localityName = m_localityRegion.GetName();
  if (regionName == localityName)
    return true;

  if (strings::StartsWith(regionName, "City of") && regionName == "City of " + localityName)
    return true;
  if (strings::EndsWith(regionName, "(city)") && regionName == localityName + " (city)")
    return true;

  auto const & regionEnglishName = region.GetName(StringUtf8Multilang::kEnglishCode);
  if (!regionEnglishName.empty())
  {
    auto const & localityEnglishName = m_localityRegion.GetName(StringUtf8Multilang::kEnglishCode);
    if (regionEnglishName == localityEnglishName)
      return true;
  }

  return false;
}

bool LocalityPointIntegrator::IsSuitableForLocalityRegionize(LevelRegion const & region) const
{
  auto const adminLevel = region.GetAdminLevel();
  if (adminLevel != AdminLevel::Unknown && adminLevel < AdminLevel::Three)
    return false;

  auto const regionPlaceType = region.GetPlaceType();
  if (regionPlaceType != PlaceType::Unknown && regionPlaceType >= PlaceType::City)
    return false;

  return true;
}

void LocalityPointIntegrator::EnlargeRegion(LevelRegion & region)
{
  LOG(LDEBUG, ("Enclose",
               StringifyPlaceType(m_localityRegion.GetPlaceType()), "place point",
               m_localityRegion.GetId(), "(", GetRegionNotation(m_localityRegion), ")",
               "into", region.GetId(), "(", GetRegionNotation(region), ")"));
  if (!region.GetLabel())
    region.SetLabel(*m_localityRegion.GetLabel());
}

void LocalityPointIntegrator::RegionizeBy(LevelRegion const & region)
{
  m_localityRegion.SetPolygon(region.GetPolygon());
  m_regionizedByRegion = region;
}

void LocalityPointIntegrator::InsertInto(Node::Ptr & node)
{
  if (m_regionizedByRegion)
  {
    LOG(LDEBUG, ("Regionize",
                 StringifyPlaceType(m_localityRegion.GetPlaceType()), "place point",
                 m_localityRegion.GetId(), "(", GetRegionNotation(m_localityRegion), ")",
                 "by", m_regionizedByRegion->GetId(),
                 "(", GetRegionNotation(*m_regionizedByRegion), ")"));
  }
  else
  {
    auto const & region = node->GetData();
    LOG(LDEBUG, ("Insert around",
                 StringifyPlaceType(m_localityRegion.GetPlaceType()), "place point",
                 m_localityRegion.GetId(), "(", GetRegionNotation(m_localityRegion), ")",
                 "into", region.GetId(), "(", GetRegionNotation(region), ")"));

    EmboundBy(region);
  }

  RegionsBuilder::InsertIntoSubtree(node, std::move(m_localityRegion), m_countrySpecifier);
}

void LocalityPointIntegrator::EmboundBy(LevelRegion const & region)
{
  auto const & regionPolygon = *region.GetPolygon();
  auto const & localityPolygon = *m_localityRegion.GetPolygon();
  auto intersection = boost::geometry::model::multi_polygon<BoostPolygon>{};
  boost::geometry::intersection(regionPolygon, localityPolygon, intersection);
  // Select one with label point.
  for (auto & polygon : intersection)
  {
    if (!boost::geometry::covered_by(m_localityRegion.GetCenter(), polygon))
      continue;

    // Skip error in boost::geometry::intersection(): there are A and B when
    // intersection(A, B) != null but intersection(A, B) != intersection(A, intersection(A, B))
    // or !covered_by(intersection(A, B), A).
    auto checkLocalityRegion = m_localityRegion;
    checkLocalityRegion.SetPolygon(std::make_shared<BoostPolygon>(std::move(polygon)));
    if (0 < RegionsBuilder::CompareAffiliation(region, checkLocalityRegion,
                                               m_countrySpecifier))
    {
      m_localityRegion = checkLocalityRegion;
      return;
    }
  }

  LOG(LWARNING, ("Failed to embound",
                 StringifyPlaceType(m_localityRegion.GetPlaceType()), "place",
                 m_localityRegion.GetId(), "(", GetRegionNotation(m_localityRegion), ")",
                 "by", region.GetId(), "(", GetRegionNotation(region), ")"));
}

// static
LevelRegion LocalityPointIntegrator::MakeAroundRegion(PlacePoint const & localityPoint,
    CountrySpecifier const & countrySpecifier)
{
  auto const placeType = localityPoint.GetPlaceType();
  auto const radius = GetRadiusByPlaceType(placeType);
  auto polygon = MakePolygonWithRadius(localityPoint.GetPosition(), radius);
  auto region = LevelRegion{
      PlaceLevel::Locality,
      {localityPoint.GetMultilangName(), localityPoint.GetRegionData(), std::move(polygon)}};
  region.SetLabel(localityPoint);
  return region;
}

// static
double LocalityPointIntegrator::GetRadiusByPlaceType(PlaceType place)
{
  // Based on average radiuses of OSM place polygons.
  switch (place)
  {
  case PlaceType::City:
    return 0.078;
  case PlaceType::Town:
    return 0.033;
  case PlaceType::Village:
    return 0.013;
  case PlaceType::Hamlet:
    return 0.0067;
  case PlaceType::IsolatedDwelling:
    return 0.0035;
  default:
    UNREACHABLE();
  }
  UNREACHABLE();
}

// static
std::shared_ptr<BoostPolygon> LocalityPointIntegrator::MakePolygonWithRadius(
    BoostPoint const & point, double radius, size_t numPoints)
{
  boost::geometry::strategy::buffer::point_circle point_strategy(numPoints);
  boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(radius);

  static boost::geometry::strategy::buffer::join_round const join_strategy;
  static boost::geometry::strategy::buffer::end_round const end_strategy;
  static boost::geometry::strategy::buffer::side_straight const side_strategy;

  boost::geometry::model::multi_polygon<BoostPolygon> result;
  boost::geometry::buffer(point, result, distance_strategy, side_strategy, join_strategy,
                          end_strategy, point_strategy);
  CHECK_EQUAL(result.size(), 1, ());
  return std::make_shared<BoostPolygon>(std::move(result.front()));
}
}  // namespace regions
}  // namespace generator
