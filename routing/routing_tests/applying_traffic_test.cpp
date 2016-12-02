#include "testing/testing.hpp"

#include "routing/car_model.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_starter.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "traffic/traffic_info.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include "routing/base/astar_algorithm.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace
{
using namespace routing;
using namespace routing_test;
using namespace traffic;

// @TODO(bykoianko) When PR with applying restricions is merged BuildXXGraph()
// should be moved to "routing/routing_tests/index_graph_tools.hpp" and it should
// be used by applying_traffic_test.cpp and cumulative_restriction_test.cpp.

//                        Finish
//  3        *              *
//             ↖          ↗
//              F5     F6
//                ↖   ↗
// 2 *              *
//    ↖          ↗   ↖
//      F2      F3      F4
//        ↖  ↗           ↖
// 1        *               *
//        ↗  ↖             ^
//      F0      F1          F8
//    ↗          ↖         |
// 0 *              *--F7--->*
//   0       1      2       3
//                Start
// Note. This graph contains of 9 one segment directed features.
unique_ptr<IndexGraph> BuildXXGraph(shared_ptr<EdgeEstimator> estimator)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(1 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(2 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(3 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {1.0, 3.0}}));
  loader->AddRoad(6 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {3.0, 3.0}}));
  loader->AddRoad(7 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(8 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {3.0, 1.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{1, 0}, {7, 0}}),                         /* joint at point (2, 0) */
      MakeJoint({{0, 1}, {1, 1}, {2, 0}, {3, 0}}),         /* joint at point (1, 1) */
      MakeJoint({{2, 1}}),                                 /* joint at point (0, 2) */
      MakeJoint({{3, 1}, {4, 1}, {5, 0}, {6, 0}}),         /* joint at point (2, 2) */
      MakeJoint({{4, 0}, {8, 1}}),                         /* joint at point (3, 1) */
      MakeJoint({{5, 1}}),                                 /* joint at point (1, 3) */
      MakeJoint({{6, 1}}),                                 /* joint at point (3, 3) */
      MakeJoint({{7, 1}, {8, 0}}),                         /* joint at point (3, 0) */
  };

  unique_ptr<IndexGraph> graph = make_unique<IndexGraph>(move(loader), estimator);
  graph->Import(joints);
  return graph;
}

class TrafficInfoGetterTest : public TrafficInfoGetter
{
public:
  TrafficInfoGetterTest(shared_ptr<TrafficInfo> trafficInfo = shared_ptr<TrafficInfo>())
    : m_trafficInfo(trafficInfo) {}

  // TrafficInfoGetter overrides:
  shared_ptr<traffic::TrafficInfo> GetTrafficInfo(MwmSet::MwmId const &) const override
  {
    return m_trafficInfo;
  }

private:
  shared_ptr<TrafficInfo> m_trafficInfo;
};

class ApplyingTrafficTest
{
public:
  ApplyingTrafficTest() { classificator::Load(); }

  void SetEstimator(TrafficInfo::Coloring && coloring)
  {
    m_trafficGetter = make_unique<TrafficInfoGetterTest>(make_shared<TrafficInfo>(move(coloring)));
    m_estimator = EdgeEstimator::CreateForCar(*make_shared<CarModelFactory>()->GetVehicleModel(),
                                              *m_trafficGetter);
  }

  shared_ptr<EdgeEstimator> GetEstimator() const { return m_estimator; }

private:
  unique_ptr<TrafficInfoGetter> m_trafficGetter;
  shared_ptr<EdgeEstimator> m_estimator;
};

// Route through XX graph without any traffic info.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_EmptyTrafficColoring)
{
  SetEstimator({} /* coloring */);

  unique_ptr<IndexGraph> graph = BuildXXGraph(GetEstimator());
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through XX graph with SpeedGroup::G0 on F3.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_G0onF3)
{
  TrafficInfo::Coloring coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G0}};
  SetEstimator(move(coloring));

  unique_ptr<IndexGraph> graph = BuildXXGraph(GetEstimator());
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  GetEstimator()->Start(MwmSet::MwmId());
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through XX graph with SpeedGroup::G0 in reverse direction on F3.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_G0onF3ReverseDir)
{
  TrafficInfo::Coloring coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kReverseDirection},
       SpeedGroup::G0}};
  SetEstimator(move(coloring));

  unique_ptr<IndexGraph> graph = BuildXXGraph(GetEstimator());
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {3, 3}};
  GetEstimator()->Start(MwmSet::MwmId());
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through XX graph SpeedGroup::G1 on F3 and F6, SpeedGroup::G4 on F8 and F4.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_G0onF3andF6andG4onF8andF4)
{
  TrafficInfo::Coloring coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G0},
      {{6 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G0},
      {{8 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G4},
      {{7 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G4}};
  SetEstimator(move(coloring));

  unique_ptr<IndexGraph> graph = BuildXXGraph(GetEstimator());
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  GetEstimator()->Start(MwmSet::MwmId());
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}
}  // namespace
