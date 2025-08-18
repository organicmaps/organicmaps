#include "generator/generator_tests_support/routing_helpers.hpp"

#include "testing/testing.hpp"

#include "generator/gen_mwm_info.hpp"

#include "routing/traffic_stash.hpp"

#include "routing_common/car_model.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"

#include <utility>

namespace generator
{
void ReEncodeOsmIdsToFeatureIdsMapping(std::string const & mappingContent, std::string const & outputFilePath)
{
  strings::SimpleTokenizer lineIter(mappingContent, "\n\r" /* line delimiters */);

  std::vector<std::pair<base::GeoObjectId, uint32_t>> osmIdsToFeatureIds;
  for (; lineIter; ++lineIter)
  {
    auto const & line = *lineIter;
    strings::SimpleTokenizer idIter(line, ", \t" /* id delimiters */);
    uint64_t osmId = 0;
    TEST(idIter, ());
    TEST(strings::to_uint(*idIter, osmId), ("Cannot convert to uint64_t:", *idIter));
    TEST(idIter, ("Wrong feature ids to osm ids mapping."));
    ++idIter;

    uint32_t featureId = 0;
    TEST(idIter, ());
    TEST(strings::to_uint(*idIter, featureId), ("Cannot convert to uint:", *idIter));
    osmIdsToFeatureIds.emplace_back(base::MakeOsmWay(osmId), featureId);
    ++idIter;
    TEST(!idIter, ());
  }

  FileWriter osm2ftWriter(outputFilePath);
  rw::WriteVectorOfPOD(osm2ftWriter, osmIdsToFeatureIds);
}
}  // namespace generator

namespace routing
{
void TestGeometryLoader::Load(uint32_t featureId, RoadGeometry & road)
{
  auto const it = m_roads.find(featureId);
  if (it == m_roads.cend())
    return;

  road = it->second;
}

void TestGeometryLoader::AddRoad(uint32_t featureId, bool oneWay, float speed, RoadGeometry::Points const & points)
{
  auto const it = m_roads.find(featureId);
  CHECK(it == m_roads.end(), ("Already contains feature", featureId));
  m_roads[featureId] = RoadGeometry(oneWay, speed, speed, points);
  m_roads[featureId].SetPassThroughAllowedForTests(true);
}

void TestGeometryLoader::SetPassThroughAllowed(uint32_t featureId, bool passThroughAllowed)
{
  auto const it = m_roads.find(featureId);
  CHECK(it != m_roads.end(), ("No feature", featureId));
  m_roads[featureId].SetPassThroughAllowedForTests(passThroughAllowed);
}

std::shared_ptr<EdgeEstimator> CreateEstimatorForCar(std::shared_ptr<TrafficStash> trafficStash)
{
  auto const carModel = CarModelFactory({}).GetVehicleModel();
  return EdgeEstimator::Create(VehicleType::Car, *carModel, trafficStash, nullptr /* DataSource */,
                               nullptr /* NumMwmIds */);
}

std::shared_ptr<EdgeEstimator> CreateEstimatorForCar(traffic::TrafficCache const & trafficCache)
{
  auto numMwmIds = std::make_shared<NumMwmIds>();
  auto stash = std::make_shared<TrafficStash>(trafficCache, numMwmIds);
  return CreateEstimatorForCar(stash);
}

Joint MakeJoint(std::vector<RoadPoint> const & points)
{
  Joint joint;
  for (auto const & point : points)
    joint.AddPoint(point);

  return joint;
}

std::unique_ptr<IndexGraph> BuildIndexGraph(std::unique_ptr<TestGeometryLoader> geometryLoader,
                                            std::shared_ptr<EdgeEstimator> estimator, std::vector<Joint> const & joints)
{
  auto graph = std::make_unique<IndexGraph>(std::make_shared<Geometry>(std::move(geometryLoader)), estimator);
  graph->Import(joints);
  return graph;
}
}  // namespace routing
