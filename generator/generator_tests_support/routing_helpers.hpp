#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/joint.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace generator
{
/// \brief Generates a binary file by |mappingContent| with mapping from osm ids to feature ids.
/// \param mappingContent a string with lines with mapping from osm id to feature id (one to one).
/// For example
/// 10, 1,
/// 20, 2
/// 30, 3,
/// 40, 4
/// \param outputFilePath full path to an output file where the mapping is saved.
void ReEncodeOsmIdsToFeatureIdsMapping(std::string const & mappingContent, std::string const & outputFilePath);
}  // namespace generator

namespace traffic
{
class TrafficCache;
}

namespace routing
{
class TestGeometryLoader : public GeometryLoader
{
public:
  // GeometryLoader overrides:
  void Load(uint32_t featureId, routing::RoadGeometry & road) override;

  void AddRoad(uint32_t featureId, bool oneWay, float speed, routing::RoadGeometry::Points const & points);

  void SetPassThroughAllowed(uint32_t featureId, bool passThroughAllowed);

private:
  std::unordered_map<uint32_t, RoadGeometry> m_roads;
};

std::shared_ptr<EdgeEstimator> CreateEstimatorForCar(traffic::TrafficCache const & trafficCache);
std::shared_ptr<EdgeEstimator> CreateEstimatorForCar(std::shared_ptr<TrafficStash> trafficStash);

Joint MakeJoint(std::vector<routing::RoadPoint> const & points);

std::unique_ptr<IndexGraph> BuildIndexGraph(std::unique_ptr<TestGeometryLoader> geometryLoader,
                                            std::shared_ptr<EdgeEstimator> estimator,
                                            std::vector<Joint> const & joints);
}  // namespace routing
