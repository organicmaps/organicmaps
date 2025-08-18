#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/fake_ending.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/routing_session.hpp"
#include "routing/traffic_stash.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include "routing/base/astar_algorithm.hpp"

#include <memory>
#include <vector>

namespace applying_traffic_test
{
using namespace routing;
using namespace routing_test;
using namespace std;
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
//                  ^
//                  F9
//                  |
//-1                *
//   0       1      2       3
//                Start
//
// Note. This graph consists of 10 one segment directed features.
unique_ptr<WorldGraph> BuildXXGraph(shared_ptr<EdgeEstimator> estimator)
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
  loader->AddRoad(9 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, -1.0}, {2.0, 0.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{1, 0}, {7, 0}, {9, 1}}),                 /* joint at point (2, 0) */
      MakeJoint({{0, 1}, {1, 1}, {2, 0}, {3, 0}}),         /* joint at point (1, 1) */
      MakeJoint({{2, 1}}),                                 /* joint at point (0, 2) */
      MakeJoint({{3, 1}, {4, 1}, {5, 0}, {6, 0}}),         /* joint at point (2, 2) */
      MakeJoint({{4, 0}, {8, 1}}),                         /* joint at point (3, 1) */
      MakeJoint({{5, 1}}),                                 /* joint at point (1, 3) */
      MakeJoint({{6, 1}}),                                 /* joint at point (3, 3) */
      MakeJoint({{7, 1}, {8, 0}}),                         /* joint at point (3, 0) */
  };

  return BuildWorldGraph(std::move(loader), estimator, joints);
}

class ApplyingTrafficTest
{
public:
  ApplyingTrafficTest()
  {
    classificator::Load();
    auto numMwmIds = make_shared<NumMwmIds>();
    m_trafficStash = make_shared<TrafficStash>(m_trafficCache, numMwmIds);
    m_estimator = CreateEstimatorForCar(m_trafficStash);
  }

  void SetTrafficColoring(shared_ptr<TrafficInfo::Coloring const> coloring)
  {
    m_trafficStash->SetColoring(kTestNumMwmId, coloring);
  }

  shared_ptr<EdgeEstimator> GetEstimator() const { return m_estimator; }

  shared_ptr<TrafficStash> GetTrafficStash() const { return m_trafficStash; }

private:
  TrafficCache m_trafficCache;
  shared_ptr<EdgeEstimator> m_estimator;
  shared_ptr<TrafficStash> m_trafficStash;
};

using Algorithm = AStarAlgorithm<Segment, SegmentEdge, RouteWeight>;

// Route through XX graph without any traffic info.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_EmptyTrafficColoring)
{
  TEST(!GetTrafficStash()->Has(kTestNumMwmId), ());

  unique_ptr<WorldGraph> graph = BuildXXGraph(GetEstimator());
  auto const start = MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2.0, -1.0), *graph);
  auto const finish = MakeFakeEnding(6, 0, m2::PointD(3.0, 3.0), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {1, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through XX graph with SpeedGroup::G0 on F3.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_G0onF3)
{
  TrafficInfo::Coloring const coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G0}};
  SetTrafficColoring(make_shared<TrafficInfo::Coloring const>(coloring));

  unique_ptr<WorldGraph> graph = BuildXXGraph(GetEstimator());
  auto const start = MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2.0, -1.0), *graph);
  auto const finish = MakeFakeEnding(6, 0, m2::PointD(3.0, 3.0), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through XX graph with SpeedGroup::TempBlock on F3.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_TempBlockonF3)
{
  TrafficInfo::Coloring const coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::TempBlock}};
  SetTrafficColoring(make_shared<TrafficInfo::Coloring const>(coloring));

  unique_ptr<WorldGraph> graph = BuildXXGraph(GetEstimator());
  auto const start = MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2.0, -1.0), *graph);
  auto const finish = MakeFakeEnding(6, 0, m2::PointD(3.0, 3.0), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through XX graph with SpeedGroup::G0 in reverse direction on F3.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_G0onF3ReverseDir)
{
  TrafficInfo::Coloring const coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kReverseDirection}, SpeedGroup::G0}};
  SetTrafficColoring(make_shared<TrafficInfo::Coloring const>(coloring));

  unique_ptr<WorldGraph> graph = BuildXXGraph(GetEstimator());
  auto const start = MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2.0, -1.0), *graph);
  auto const finish = MakeFakeEnding(6, 0, m2::PointD(3.0, 3.0), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {1, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through XX graph SpeedGroup::G1 on F3 and F6, SpeedGroup::G4 on F8 and F4.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_G0onF3andF6andG4onF8andF4)
{
  TrafficInfo::Coloring const coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G0},
      {{6 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G0},
      {{8 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G4},
      {{7 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G4}};
  SetTrafficColoring(make_shared<TrafficInfo::Coloring const>(coloring));

  unique_ptr<WorldGraph> graph = BuildXXGraph(GetEstimator());
  auto const start = MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2.0, -1.0), *graph);
  auto const finish = MakeFakeEnding(6, 0, m2::PointD(3.0, 3.0), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through XX graph with changing traffic.
UNIT_CLASS_TEST(ApplyingTrafficTest, XXGraph_ChangingTraffic)
{
  // No trafic at all.
  TEST(!GetTrafficStash()->Has(kTestNumMwmId), ());

  unique_ptr<WorldGraph> graph = BuildXXGraph(GetEstimator());
  auto const start = MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2.0, -1.0), *graph);
  auto const finish = MakeFakeEnding(6, 0, m2::PointD(3.0, 3.0), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const noTrafficGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {1, 1}, {2, 2}, {3, 3}};
  {
    TestRouteGeometry(*starter, Algorithm::Result::OK, noTrafficGeom);
  }

  // Heavy traffic (SpeedGroup::G0) on F3.
  TrafficInfo::Coloring const coloringHeavyF3 = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G0}};
  SetTrafficColoring(make_shared<TrafficInfo::Coloring const>(coloringHeavyF3));
  {
    vector<m2::PointD> const heavyF3Geom = {{2 /* x */, -1 /* y */}, {2, 0}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
    TestRouteGeometry(*starter, Algorithm::Result::OK, heavyF3Geom);
  }

  // Overloading traffic jam on F3. Middle traffic (SpeedGroup::G3) on F1, F3, F4, F7 and F8.
  TrafficInfo::Coloring const coloringMiddleF1F3F4F7F8 = {
      {{1 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G3},
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G3},
      {{4 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G3},
      {{7 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G3},
      {{8 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection}, SpeedGroup::G3}};
  SetTrafficColoring(make_shared<TrafficInfo::Coloring const>(coloringMiddleF1F3F4F7F8));
  {
    TestRouteGeometry(*starter, Algorithm::Result::OK, noTrafficGeom);
  }
}
}  // namespace applying_traffic_test
