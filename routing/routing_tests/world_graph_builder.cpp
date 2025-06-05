#include "routing/routing_tests/world_graph_builder.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/joint.hpp"
#include "routing/single_vehicle_world_graph.hpp"

#include "traffic/traffic_cache.hpp"

#include <utility>
#include <vector>

namespace routing_test
{
using namespace routing;

//                             Finish
//                               *
//                               ^
//                               |
//                               F7
//                               |
//                               *
//                               ^
//                               |
//                               F6
//                               |
// Start * -- F0 --> * -- F1 --> * <-- F2 --> * <-- F3 --> *
//                              | ^
//                              | |
//                             F4 F5
//                              | |
//                              ⌄ |
//                               *
std::unique_ptr<SingleVehicleWorldGraph> BuildCrossGraph()
{
  std::unique_ptr<TestGeometryLoader> loader = std::make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(1 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(2 /* featureId */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.9999, 0.0}}));
  loader->AddRoad(3 /* featureId */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.9999, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, -1.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, -1.0}, {1.0, 0.0}}));
  loader->AddRoad(6 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(7 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {1.0, 2.0}}));

  std::vector<Joint> const joints = {MakeJoint({{0, 1}, {1, 0}}), MakeJoint({{1, 1}, {2, 0}, {4, 0}, {5, 1}, {6, 0}}),
                                     MakeJoint({{2, 1}, {3, 0}}), MakeJoint({{4, 1}, {5, 0}}),
                                     MakeJoint({{6, 1}, {7, 0}})};

  traffic::TrafficCache const trafficCache;
  std::shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);

  return BuildWorldGraph(std::move(loader), estimator, joints);
}

//
// 2    * -- F2 --> * -- F2 --> * -- F2 --> * -- F5 --> *
//      ↑                                 ↗
//      F2                            F3
//      ↑                          ↗
// 1    * <-- F0 --> * <-- F1 --> *
//                                 ↘
//                                    F4
//                                        ↘
// 0                                        *
//
//     -1           0            1          2           3
//
std::unique_ptr<SingleVehicleWorldGraph> BuildTestGraph()
{
  std::unique_ptr<TestGeometryLoader> loader = std::make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 1.0}, {-1.0, 1.0}}));
  loader->AddRoad(1 /* featureId */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 1.0}, {1.0, 1.0}}));
  loader->AddRoad(2 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 1.0}, {-1.0, 2.0}, {0.0, 2.0}, {1.0, 2.0}, {2.0, 2.0}}));
  loader->AddRoad(3 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {3.0, 2.0}}));

  std::vector<Joint> const joints = {MakeJoint({{1, 0}, {0, 0}}),           // (0, 1)
                                     MakeJoint({{0, 1}, {2, 0}}),           // (-1, 1)
                                     MakeJoint({{3, 1}, {2, 4}, {5, 0}}),   // (2, 2)
                                     MakeJoint({{1, 1}, {3, 0}, {4, 0}})};  // (1, 1)

  traffic::TrafficCache const trafficCache;
  std::shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);

  return BuildWorldGraph(std::move(loader), estimator, joints);
}
}  // namespace routing_test
