#include "generator/routing_city_boundaries_processor.hpp"

#include "generator/collector_routing_city_boundaries.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/area_on_earth.hpp"
#include "geometry/circle_on_earth.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include "std/boost_geometry.hpp"
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace generator
{
namespace routing_city_boundaries
{
using LocalityData = RoutingCityBoundariesWriter::LocalityData;

std::unordered_map<uint64_t, LocalityData> LoadNodeToLocalityData(std::string const & filename)
{
  FileReader reader(filename);
  ReaderSource<FileReader> source(reader);
  uint64_t n = 0;
  source.Read(&n, sizeof(n));

  std::unordered_map<uint64_t, LocalityData> result;
  for (size_t i = 0; i < n; ++i)
  {
    uint64_t nodeId = 0;
    source.Read(&nodeId, sizeof(nodeId));

    LocalityData localityData = LocalityData::Deserialize(source);
    result[nodeId] = localityData;
  }

  return result;
}

std::unordered_map<uint64_t, std::vector<feature::FeatureBuilder>> LoadNodeToBoundariesData(
    std::string const & filename)
{
  using MinAccuracy = feature::serialization_policy::MinSize;
  FileReader reader(filename);
  ReaderSource<FileReader> source(reader);
  uint64_t n = 0;
  source.Read(&n, sizeof(n));

  std::unordered_map<uint64_t, std::vector<feature::FeatureBuilder>> result;
  for (size_t i = 0; i < n; ++i)
  {
    uint64_t nodeId = 0;
    source.Read(&nodeId, sizeof(nodeId));

    feature::FeatureBuilder featureBuilder;
    feature::ReadFromSourceRawFormat<MinAccuracy>(source, featureBuilder);
    result[nodeId].emplace_back(std::move(featureBuilder));
  }

  return result;
}

double AreaOnEarth(std::vector<m2::PointD> const & points)
{
  namespace bg = boost::geometry;
  using LonLatCoords = bg::cs::spherical_equatorial<bg::degree>;
  bg::model::ring<bg::model::point<double, 2, LonLatCoords>> sphericalPolygon;

  auto const addPoint = [&sphericalPolygon](auto const & point) {
    auto const latlon = mercator::ToLatLon(point);
    sphericalPolygon.emplace_back(latlon.m_lon, latlon.m_lat);
  };

  sphericalPolygon.reserve(points.size());
  for (auto point : points)
    addPoint(point);

  addPoint(points.front());

  bg::strategy::area::spherical<> areaCalculationStrategy(ms::kEarthRadiusMeters);

  double const area = bg::area(sphericalPolygon, areaCalculationStrategy);
  return fabs(area);
}

std::pair<feature::FeatureBuilder, double> GetBoundaryWithSmallestArea(
    std::vector<feature::FeatureBuilder> const & boundaries)
{
  size_t bestIndex = 0;
  double minArea = std::numeric_limits<double>::max();
  for (size_t i = 0; i < boundaries.size(); ++i)
  {
    auto const & geometry = boundaries[i].GetOuterGeometry();
    double const area = AreaOnEarth(geometry);

    if (minArea > area)
    {
      minArea = area;
      bestIndex = i;
    }
  }

  return {boundaries[bestIndex], minArea};
}
}  // namespace routing_city_boundaries

RoutingCityBoundariesProcessor::RoutingCityBoundariesProcessor(std::string tmpFilename,
                                                               std::string dumpFilename)
  : m_tmpFilename(std::move(tmpFilename))
  , m_dumpFilename(std::move(dumpFilename))
{
}

void RoutingCityBoundariesProcessor::ProcessDataFromCollector()
{
  using namespace routing_city_boundaries;
  auto const nodeOsmIdToLocalityData = LoadNodeToLocalityData(
      RoutingCityBoundariesWriter::GetNodeToLocalityDataFilename(m_tmpFilename));

  auto const nodeOsmIdToBoundaries = LoadNodeToBoundariesData(
      RoutingCityBoundariesWriter::GetNodeToBoundariesFilename(m_tmpFilename));

  FileWriter boundariesWriter(m_dumpFilename, FileWriter::Op::OP_APPEND);

  uint32_t pointToCircle = 0;
  uint32_t matchedBoundary = 0;

  for (auto const & item : nodeOsmIdToLocalityData)
  {
    uint64_t const nodeOsmId = item.first;

    auto const & localityData = item.second;
    if (localityData.m_population == 0)
      continue;

    double const radiusMeters =
        ftypes::GetRadiusByPopulationForRouting(localityData.m_population, localityData.m_place);
    double const areaUpperBound = ms::CircleAreaOnEarth(radiusMeters);

    feature::FeatureBuilder feature;

    auto const boundariesIt = nodeOsmIdToBoundaries.find(nodeOsmId);
    if (boundariesIt != nodeOsmIdToBoundaries.cend())
    {
      auto const & boundaries = boundariesIt->second;
      double bestFeatureBuilderArea = 0.0;
      std::tie(feature, bestFeatureBuilderArea) = GetBoundaryWithSmallestArea(boundaries);

      if (bestFeatureBuilderArea <= areaUpperBound)
      {
        ++matchedBoundary;
        rw::WriteVectorOfPOD(boundariesWriter, feature.GetOuterGeometry());
        continue;
      }
    }

    auto const circleGeometry = ms::CreateCircleGeometryOnEarth(
        mercator::ToLatLon(localityData.m_position), radiusMeters, 30.0 /* angleStepDegree */);

    ++pointToCircle;
    rw::WriteVectorOfPOD(boundariesWriter, circleGeometry);
  }

  LOG(LINFO, (pointToCircle, "places were transformed to circle."));
  LOG(LINFO, (matchedBoundary, "boundaries were approved as city/town/village boundary."));
}
}  // namespace generator
