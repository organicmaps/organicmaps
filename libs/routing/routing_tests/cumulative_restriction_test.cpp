#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/fake_ending.hpp"
#include "routing/geometry.hpp"
#include "routing/restriction_loader.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing_common/car_model.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace cumulative_restriction_test
{
using namespace routing;
using namespace routing_test;
using namespace std;

//               Finish
//  3               *
//                  ^
//                  F5
//                  |
// 2 *              *
//    ↖          ↗   ↖
//      F2     F3     F4
//        ↖  ↗           ↖
// 1        *               *
//        ↗  ↖
//      F0      F1
//    ↗          ↖
// 0 *              *
//   0       1      2       3
//                Start
// Note. This graph contains of 6 one segment directed features.
unique_ptr<SingleVehicleWorldGraph> BuildXYGraph()
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
                  RoadGeometry::Points({{2.0, 2.0}, {2.0, 3.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{1, 0}}),                                 /* joint at point (2, 0) */
      MakeJoint({{0, 1}, {1, 1}, {2, 0}, {3, 0}}),         /* joint at point (1, 1) */
      MakeJoint({{2, 1}}),                                 /* joint at point (0, 2) */
      MakeJoint({{3, 1}, {4, 1}, {5, 0}}),                 /* joint at point (2, 2) */
      MakeJoint({{4, 0}}),                                 /* joint at point (3, 1) */
      MakeJoint({{5, 1}}),                                 /* joint at point (2, 3) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

using Algorithm = AStarAlgorithm<Segment, SegmentEdge, RouteWeight>;

// Route through XY graph without any restrictions.
UNIT_TEST(XYGraph)
{
  unique_ptr<WorldGraph> graph = BuildXYGraph();
  auto const start = MakeFakeEnding(1 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *graph);
  auto const finish = MakeFakeEnding(5, 0, m2::PointD(2, 3), *graph);
  auto starter = MakeStarter(start, finish, *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {2, 3}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through XY graph with one restriciton (type only) from F1 to F3.
UNIT_CLASS_TEST(RestrictionTest, XYGraph_RestrictionF1F3Only)
{
  Init(BuildXYGraph());
  RestrictionVec restrictionsOnly = {{1 /* feature from */, 3 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {2, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(1 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(5, 0, m2::PointD(2, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Route through XY graph with one restriction (type only) from F3 to F5.
UNIT_CLASS_TEST(RestrictionTest, XYGraph_RestrictionF3F5Only)
{
  Init(BuildXYGraph());
  RestrictionVec restrictionsOnly = {{3 /* feature from */, 5 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {2, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(1 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(5, 0, m2::PointD(2, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Cumulative case. Route through XY graph with two restricitons (type only) applying
// in all possible orders.
UNIT_CLASS_TEST(RestrictionTest, XYGraph_PermutationsF3F5OnlyF1F3Only)
{
  Init(BuildXYGraph());
  RestrictionVec restrictionsOnly = {{1 /* feature from */, 3 /* feature to */},
                                     {3 /* feature from */, 5 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {2, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(1 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(5, 0, m2::PointD(2, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Cumulative case. Route through XY graph with two restricitons (type only and type no) applying
// in all possible orders.
UNIT_CLASS_TEST(RestrictionTest, XYGraph_PermutationsF3F5OnlyAndF0F2No)
{
  Init(BuildXYGraph());

  RestrictionVec restrictionsNo = {{1 /* feature from */, 2 /* feature to */}};

  RestrictionVec restrictionsOnly = {{3 /* feature from */, 5 /* feature to */}};
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {2, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(1 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(5, 0, m2::PointD(2, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Cumulative case. Trying to build route through XY graph with two restricitons applying
// according to the order. First from F3 to F5 (type only)
// and then and from F1 to F3 (type no).
UNIT_CLASS_TEST(RestrictionTest, XYGraph_RestrictionF3F5OnlyAndF1F3No)
{
  Init(BuildXYGraph());
  RestrictionVec restrictionsNo = {{1 /* feature from */, 3 /* feature to */}};
  RestrictionVec restrictionsOnly = {{3 /* feature from */, 5 /* feature to */}};
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  TestRestrictions({} /* expectedGeom */, Algorithm::Result::NoPath,
                   MakeFakeEnding(1 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(5, 0, m2::PointD(2, 3), *m_graph), std::move(restrictionsNo), *this);
}

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
//        ↗  ↖              ^
//      F0      F1          F8
//    ↗          ↖          |
// 0 *              *--F7--->*
//                  ^
//                  F9
//                  |
//-1                *
//   0       1      2       3
//                Start
// Note. This graph contains of 9 one segment directed features.
unique_ptr<SingleVehicleWorldGraph> BuildXXGraph()
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
      MakeJoint({{9, 0}}),                                 /* joint at point (2, -1) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// 2 *              *
//               ↗    ↘
//             F4       F5
//           ↗             ↘
// 1        *                *
//           ↖               ↑
//             F2            F3
//               ↖           ↓ <-- Finish
// 0 *              *--F1--->*
//                  ^
//                  F0
//                  |
//-1                *
//   0       1      2        3
//                Start
// Note. This graph contains of 9 one segment directed features.
unique_ptr<SingleVehicleWorldGraph> BuildCubeGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, -1.0}, {2.0, 0.0}}));
  loader->AddRoad(1 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(2 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(3 /* featureId */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 1.0}, {3.0, 0.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {3.0, 1.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (2, -1) */
      MakeJoint({{0, 1}, {1, 0}, {2, 0}}),                 /* joint at point (2, 0) */
      MakeJoint({{2, 1}, {4, 0}}),                         /* joint at point (1, 1) */
      MakeJoint({{4, 1}, {5, 0}}),                         /* joint at point (2, 2) */
      MakeJoint({{5, 1}, {3, 0}}),                         /* joint at point (3, 1) */
      MakeJoint({{1, 1}, {3, 1}}),                         /* joint at point (3, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// Route through XY graph without any restrictions.
UNIT_CLASS_TEST(RestrictionTest, XXGraph)
{
  Init(BuildXXGraph());
  RestrictionVec restrictions = {};
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {1, 1}, {2, 2}, {3, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, -1), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(3, 3), *m_graph), std::move(restrictions), *this);
}

// Cumulative case. Route through XX graph with two restricitons (type only) applying
// in all possible orders.
UNIT_CLASS_TEST(RestrictionTest, XXGraph_PermutationsF1F3OnlyAndF3F6Only)
{
  Init(BuildXXGraph());
  RestrictionVec restrictionsNo;
  RestrictionVec restrictionsOnly = {{1 /* feature from */, 3 /* feature to */},
                                     {3 /* feature from */, 6 /* feature to */}};
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {1, 1}, {2, 2}, {3, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, -1), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(3, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Route through XX graph with one restriciton (type no) from F1 to F3.
UNIT_CLASS_TEST(RestrictionTest, XXGraph_RestrictionF1F3No)
{
  Init(BuildXXGraph());
  RestrictionVec restrictionsNo = {{1 /* feature from */, 3 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, -1), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(3, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Cumulative case. Route through XX graph with four restricitons of different types applying
// in all possible orders.
UNIT_CLASS_TEST(RestrictionTest, XXGraph_PermutationsF1F3NoF7F8OnlyF8F4OnlyF4F6Only)
{
  Init(BuildXXGraph());
  RestrictionVec restrictionsNo = {{1 /* feature from */, 3 /* feature to */}};

  RestrictionVec restrictionsOnly = {{4 /* feature from */, 6 /* feature to */},
                                     {7 /* feature from */, 8 /* feature to */},
                                     {8 /* feature from */, 4 /* feature to */}};

  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, -1 /* y */}, {2, 0}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(9 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, -1), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(3, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, XXGraph_CheckOnlyRestriction)
{
  Init(BuildCubeGraph());

  m2::PointD const start(2.0, -1.0);
  m2::PointD const finish(3.0, 0.2);
  auto const test = [&](vector<m2::PointD> const & expectedGeom, RestrictionVec && restrictionsNo)
  {
    TestRestrictions(
        expectedGeom, Algorithm::Result::OK, MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, start, *m_graph),
        MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, finish, *m_graph), std::move(restrictionsNo), *this);
  };

  RestrictionVec restrictionsOnly = {
      {0 /* feature from */, 2 /* feature to */},
  };
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  // Check that without restrictions we can find path better.
  test({start, {2, 0}, {1, 1}, {2, 2}, {3, 1}, finish}, std::move(restrictionsNo));
  test({start, {2, 0}, {3, 0}, finish}, RestrictionVec());
}
}  // namespace cumulative_restriction_test
