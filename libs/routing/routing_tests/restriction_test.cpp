#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"
#include "routing/routing_tests/world_graph_builder.hpp"

#include "routing/fake_ending.hpp"
#include "routing/geometry.hpp"
#include "routing/restriction_loader.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace restriction_test
{
using namespace routing;
using namespace routing_test;
using namespace std;

using Algorithm = AStarAlgorithm<Segment, SegmentEdge, RouteWeight>;

UNIT_CLASS_TEST(RestrictionTest, CrossGraph_NoUTurn)
{
  Init(BuildCrossGraph());
  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *m_graph),
             MakeFakeEnding(7, 0, m2::PointD(1, 2), *m_graph));

  vector<m2::PointD> const expectedGeom = {{-1.0 /* x */, 0.0 /* y */}, {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}};
  TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, CrossGraph_UTurn)
{
  Init(BuildCrossGraph());
  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *m_graph),
             MakeFakeEnding(7, 0, m2::PointD(1, 2), *m_graph));

  RestrictionVec restrictionsNo = {{1 /* feature from */, 6 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{-1.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {1.0, -1.0},
                                           {1.0, 0.0},  {1.0, 1.0}, {1.0, 2.0}};

  TestRestrictions(6.0 /* expectedTime */,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *m_graph),
                   MakeFakeEnding(7, 0, m2::PointD(1, 2), *m_graph), std::move(restrictionsNo), *this);
}

// Finish
// 3 *
//   ^
//   |
//   F4
//   |
// 2 *
//   ^ ↖
//   |   F1
//   |      ↖
// 1 |        *
//   F0         ↖
//   |            F2
//   |              ↖
// 0 *<--F3---<--F3---*<--F5--* Start
//   0        1       2       3
// Note. F0, F1 and F2 are one segment features. F3 is a two segments feature.
unique_ptr<SingleVehicleWorldGraph> BuildTriangularGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 2.0}}));
  loader->AddRoad(1 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(2 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(3 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {0.0, 3.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {2.0, 0.0}}));

  vector<Joint> const joints = {
      MakeJoint({{2, 0}, {3, 0}, {5, 1}}), /* joint at point (2, 0) */
      MakeJoint({{3, 2}, {0, 0}}),         /* joint at point (0, 0) */
      MakeJoint({{2, 1}, {1, 0}}),         /* joint at point (1, 1) */
      MakeJoint({{0, 1}, {1, 1}, {4, 0}}), /* joint at point (0, 2) */
      MakeJoint({{5, 0}}),                 /* joint at point (3, 0) */
      MakeJoint({{4, 1}})                  /* joint at point (0, 3) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// Route through triangular graph without any restrictions.
UNIT_CLASS_TEST(RestrictionTest, TriangularGraph)
{
  Init(BuildTriangularGraph());

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), {}, *this);
}

// Route through triangular graph with restriction type no from feature 2 to feature 1.
UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionNoF2F1)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictionsNo = {{2 /* feature from */, 1 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(5 /* featureId */, 0 /* senmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionNoF5F2)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictionsNo = {{5 /* feature from */, 2 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionOnlyF5F3)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictionsOnly = {{{5 /* feature from */, 3 /* feature to */}}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionNoF5F2RestrictionOnlyF5F3)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictionsNo = {{5 /* feature from */, 2 /* feature to */}};

  RestrictionVec restrictionsOnly = {{5 /* feature from */, 3 /* feature to */}};
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Finish
// 3 *
//   |
//   F4
//   |
// 2 *
//   | ╲
//   F0  F2
//   |     ╲
// 1 *       *
//   |         ╲
//   F0         F2
//   |             ╲
// 0 *---F1--*--F1--*--F3---* Start
//   0       1      2       3
// Note. All features are two setments and two-way.
unique_ptr<SingleVehicleWorldGraph> BuildTwowayCornerGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(1 /* feature id */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(3 /* feature id */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(4 /* feature id */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {0.0, 3.0}}));

  vector<Joint> const joints = {
      MakeJoint({{1 /* feature id */, 2 /* point id */}, {0, 0}})
      /* joint at point (0, 0) */,
      MakeJoint({{1, 0}, {2, 0}, {3, 1}}), /* joint at point (2, 0) */
      MakeJoint({{2, 2}, {0, 2}, {4, 0}}), /* joint at point (0, 2) */
      MakeJoint({{4, 1}}),                 /* joint at point (0, 3) */
      MakeJoint({{3, 0}}),                 /* joint at point (3, 0) */
  };
  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, TwowayCornerGraph)
{
  Init(BuildTwowayCornerGraph());
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), {}, *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwowayCornerGraph_RestrictionF3F2No)
{
  Init(BuildTwowayCornerGraph());
  RestrictionVec restrictionsNo = {{3 /* feature from */, 2 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwowayCornerGraph_RestrictionF3F1Only)
{
  Init(BuildTwowayCornerGraph());
  RestrictionVec restrictionsOnly = {{3 /* feature from */, 1 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

// Finish
// 3 *
//   ^
//   |
//  F11
//   |
// 2 *<---F5----*<---F6---*
//   ^ ↖        ^ ↖       ^
//   |   F7     |   F8    |
//   |     ↖   F1     ↖  F2
//   |       ↖  |       ↖ |
// 1 F0         *         *
//   |          ^  ↖      ^
//   |         F1    F9   F2
//   |          |       ↖ |
// 0 *<----F4---*<---F3----*<--F10---* Start
//   0          1          2         3
// Note. F1 and F2 are two segments features. The others are one segment ones.
unique_ptr<SingleVehicleWorldGraph> BuildTwoSquaresGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 2.0}}));
  loader->AddRoad(1 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}}));
  loader->AddRoad(2 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {2.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(3 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(4 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(5 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 2.0}, {0.0, 2.0}}));
  loader->AddRoad(6 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {1.0, 2.0}}));
  loader->AddRoad(7 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(8 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 1.0}, {1.0, 2.0}}));
  loader->AddRoad(9 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(10 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(11 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {0.0, 3.0}}));

  vector<Joint> const joints = {
      MakeJoint({{4 /* featureId */, 1 /* pointId */}, {0, 0}}), /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {5, 1}, {7, 1}, {11, 0}}),              /* joint at point (0, 2) */
      MakeJoint({{4, 0}, {1, 0}, {3, 1}}),                       /* joint at point (1, 0) */
      MakeJoint({{5, 0}, {1, 2}, {6, 1}, {8, 1}}),               /* joint at point (1, 2) */
      MakeJoint({{3, 0}, {2, 0}, {9, 0}, {10, 1}}),              /* joint at point (2, 0) */
      MakeJoint({{2, 2}, {6, 0}}),                               /* joint at point (2, 2) */
      MakeJoint({{1, 1}, {9, 1}, {7, 0}}),                       /* joint at point (1, 1) */
      MakeJoint({{2, 1}, {8, 0}}),                               /* joint at point (2, 1) */
      MakeJoint({{10, 0}}),                                      /* joint at point (3, 0) */
      MakeJoint({{11, 1}}),                                      /* joint at point (0, 3) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph)
{
  Init(BuildTwoSquaresGraph());
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), {}, *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph_RestrictionF10F3Only)
{
  Init(BuildTwoSquaresGraph());
  RestrictionVec restrictionsOnly = {{10 /* feature from */, 3 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph_RestrictionF10F3OnlyF3F4Only)
{
  Init(BuildTwoSquaresGraph());
  RestrictionVec restrictionsOnly = {{3 /* feature from */, 4 /* feature to */},
                                     {10 /* feature from */, 3 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph_RestrictionF2F8NoRestrictionF9F1Only)
{
  Init(BuildTwoSquaresGraph());
  RestrictionVec restrictionsNo = {{2 /* feature from */, 8 /* feature to */}};    // Invalid restriction.
  RestrictionVec restrictionsOnly = {{9 /* feature from */, 1 /* feature to */}};  // Invalid restriction.
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), std::move(restrictionsNo), *this);
}

// 2      *
//        |
//        F6
//        |Finish
// 1 *-F4-*-F5-*
//   |         |
//   F2        F3
//   |         |
// 0 *---F1----*---F0---* Start
//   0         1        2
// Note 1. All features are two-way. (It's possible to std::move along any direction of the features.)
// Note 2. Any feature contains of one segment.
unique_ptr<SingleVehicleWorldGraph> BuildFlagGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(1 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 1.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 1.0}, {0.5, 1.0}}));
  loader->AddRoad(5 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.5, 1.0}, {1.0, 1.0}}));
  loader->AddRoad(6 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.5, 1.0}, {0.5, 2.0}}));

  vector<Joint> const joints = {
      MakeJoint({{1 /* feature id */, 1 /* point id */}, {2, 0}}), /* joint at point (0, 0) */
      MakeJoint({{2, 1}, {4, 0}}),                                 /* joint at point (0, 1) */
      MakeJoint({{4, 1}, {5, 0}, {6, 0}}),                         /* joint at point (0.5, 1) */
      MakeJoint({{1, 0}, {3, 0}, {0, 1}}),                         /* joint at point (1, 0) */
      MakeJoint({{3, 1}, {5, 1}}),                                 /* joint at point (1, 1) */
      MakeJoint({{6, 1}}),                                         /* joint at point (0.5, 2) */
      MakeJoint({{0, 0}}),                                         /* joint at point (2, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// Route through flag graph without any restrictions.
UNIT_TEST(FlagGraph)
{
  unique_ptr<WorldGraph> graph = BuildFlagGraph();
  auto starter = MakeStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *graph),
                             MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {0.5, 1}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through flag graph with one restriciton (type no) from F0 to F3.
UNIT_CLASS_TEST(RestrictionTest, FlagGraph_RestrictionF0F3No)
{
  Init(BuildFlagGraph());
  RestrictionVec restrictionsNo = {{0 /* feature from */, 3 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *m_graph), std::move(restrictionsNo), *this);
}

// Route through flag graph with one restriciton (type only) from F0 to F1.
UNIT_CLASS_TEST(RestrictionTest, FlagGraph_RestrictionF0F1Only)
{
  Init(BuildFlagGraph());
  RestrictionVec restrictionsNo = {{0 /* feature from */, 1 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {0.5, 1}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *m_graph), std::move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, FlagGraph_PermutationsF1F3NoF7F8OnlyF8F4OnlyF4F6Only)
{
  Init(BuildFlagGraph());
  RestrictionVec restrictionsNo = {{0 /* feature from */, 3 /* feature to */}};

  RestrictionVec restrictionsOnly = {{0 /* feature from */, 1 /* feature to */},
                                     {1 /* feature from */, 2 /* feature to */}};

  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *m_graph), std::move(restrictionsNo), *this);
}

// 1 *-F4-*-F5-*---F6---* Finish
//   |         |
//   F2        F3
//   |         |
// 0 *---F1----*---F0---* Start
//   0         1        2
// Note 1. All features except for F7 are two-way.
// Note 2. Any feature contains of one segment.
unique_ptr<SingleVehicleWorldGraph> BuildPosterGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(1 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 1.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 1.0}, {0.5, 1.0}}));
  loader->AddRoad(5 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.5, 1.0}, {1.0, 1.0}}));
  loader->AddRoad(6 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 1.0}}));

  vector<Joint> const joints = {
      MakeJoint({{1 /* feature id */, 1 /* point id */}, {2, 0}}), /* joint at point (0, 0) */
      MakeJoint({{2, 1}, {4, 0}}),                                 /* joint at point (0, 1) */
      MakeJoint({{4, 1}, {5, 0}}),                                 /* joint at point (0.5, 1) */
      MakeJoint({{1, 0}, {3, 0}, {0, 1}}),                         /* joint at point (1, 0) */
      MakeJoint({{3, 1}, {5, 1}, {6, 0}}),                         /* joint at point (1, 1) */
      MakeJoint({{0, 0}}),                                         /* joint at point (2, 0) */
      MakeJoint({{6, 1}}),                                         /* joint at point (2, 1) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// Route through poster graph without any restrictions.
UNIT_TEST(PosterGraph)
{
  unique_ptr<WorldGraph> graph = BuildPosterGraph();
  auto starter = MakeStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *graph),
                             MakeFakeEnding(6, 0, m2::PointD(2, 1), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {2, 1}};

  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// Route through poster graph with restrictions F0-F3 (type no).
UNIT_CLASS_TEST(RestrictionTest, PosterGraph_RestrictionF0F3No)
{
  Init(BuildPosterGraph());
  RestrictionVec restrictionsNo = {{0 /* feature from */, 3 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}, {1, 1}, {2, 1}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(2, 1), *m_graph), std::move(restrictionsNo), *this);
}

// Route through poster graph with restrictions F0-F1 (type only).
UNIT_CLASS_TEST(RestrictionTest, PosterGraph_RestrictionF0F1Only)
{
  Init(BuildPosterGraph());

  RestrictionVec restrictionsOnly = {{0 /* feature from */, 1 /* feature to */}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}, {1, 1}, {2, 1}};
  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
                   MakeFakeEnding(6, 0, m2::PointD(2, 1), *m_graph), std::move(restrictionsNo), *this);
}

// 1                        *--F1-->*
//                        ↗          ↘
//                      F1               F1
//                    ↗                   ↘
// 0 Start *---F3--->*---F0--->-------F0----->*---F2-->* Finish
//        -1         0        1       2       3        4
// Note. F0 is a two segments feature. F1 is a three segment one. F2 and F3 are one segment
// features.
unique_ptr<WorldGraph> BuildTwoWayGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}, {3.0, 0}}));
  loader->AddRoad(1 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 1.0}, {2.0, 1.0}, {3.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {4.0, 0.0}}));
  loader->AddRoad(3 /* feature id */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 0.0}, {0.0, 0.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}, {1, 0}, {3, 1}}), /* joint at point (0, 0) */
      MakeJoint({{0 /* feature id */, 2 /* point id */}, {1, 3}, {2, 0}}), /* joint at point (3, 0) */
      MakeJoint({{3 /* feature id */, 0 /* point id */}}),                 /* joint at point (-1, 0) */
      MakeJoint({{2 /* feature id */, 1 /* point id */}}),                 /* joint at point (4, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

UNIT_TEST(TwoWayGraph)
{
  unique_ptr<WorldGraph> graph = BuildTwoWayGraph();
  auto starter = MakeStarter(MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *graph),
                             MakeFakeEnding(2, 0, m2::PointD(4, 0), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{-1 /* x */, 0 /* y */}, {0, 0}, {1, 0}, {3, 0}, {4, 0}};

  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

// 1          *---F4----*
//            |         |
//           F2        F3
//            |         |
// 0 *<--F5---*<--F1----*<--F0---* Start
// Finish
//   0        1         2        3
// Note 1. F0, F1 and F5 are one-way features. F3, F2 and F4 are two-way features.
// Note 2. Any feature contains of one segment.
unique_ptr<SingleVehicleWorldGraph> BuildSquaresGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(1 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {2.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 1.0}, {1.0, 1.0}}));
  loader->AddRoad(5 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {0.0, 0.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (3, 0) */
      MakeJoint({{0, 1}, {3, 0}, {1, 0}}),                 /* joint at point (2, 0) */
      MakeJoint({{3, 1}, {4, 0}}),                         /* joint at point (2, 1) */
      MakeJoint({{2, 1}, {4, 1}}),                         /* joint at point (1, 1) */
      MakeJoint({{1, 1}, {2, 0}, {5, 0}}),                 /* joint at point (1, 0) */
      MakeJoint({{5, 1}})                                  /* joint at point (0, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

UNIT_TEST(SquaresGraph)
{
  unique_ptr<WorldGraph> graph = BuildSquaresGraph();
  auto starter = MakeStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *graph),
                             MakeFakeEnding(5, 0, m2::PointD(0, 0), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}};
  TestRouteGeometry(*starter, Algorithm::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, SquaresGraph_RestrictionF0F1OnlyF1F5Only)
{
  Init(BuildSquaresGraph());
  RestrictionVec restrictionsNo;
  RestrictionVec restrictionsOnly = {{0 /* feature from */, 3 /* feature to */}};

  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{3.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}, {0.0, 0.0}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
                   MakeFakeEnding(5, 0, m2::PointD(0, 0), *m_graph), std::move(restrictionsNo), *this);
}

// 0 Start *--F0--->*<--F1---*---F1---*---F1--->*---F2-->* Finish
//         0        1        2        3        4        5
// Note. F0 and F2 are one segment one-way features. F1 is a 3 segment two-way feature.
unique_ptr<SingleVehicleWorldGraph> BuildLineGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 1.0}}));
  loader->AddRoad(1 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{4.0, 0.0}, {5.0, 0.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {1, 0}}),                         /* joint at point (1, 0) */
      MakeJoint({{1, 3}, {2, 0}}),                         /* joint at point (4, 0) */
      MakeJoint({{2, 1}}),                                 /* joint at point (5, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// This test checks that despite the fact uturn on F1 is prohibited (moving from F1 to F1 is
// prohibited)
// it's still possible to std::move from F1 to F1 in straight direction.
UNIT_CLASS_TEST(RestrictionTest, LineGraph_RestrictionF1F1No)
{
  Init(BuildLineGraph());
  RestrictionVec restrictionsNo = {{1 /* feature from */, 1 /* feature to */}};
  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
                   MakeFakeEnding(2, 0, m2::PointD(5, 0), *m_graph), std::move(restrictionsNo), *this);
}

// 2 *---F2-->*
//   ^
//   F0
//   |
// 1 *---F1-->* Finish
//   ^
//   F0
//   |
// 0 *
//   0        1
//  Start
unique_ptr<SingleVehicleWorldGraph> BuildFGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(1 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 1.0}, {1.0, 1.0}}));
  loader->AddRoad(2 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {1.0, 2.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {1, 0}}),                         /* joint at point (0, 1) */
      MakeJoint({{0, 2}, {2, 0}}),                         /* joint at point (0, 2) */
      MakeJoint({{1, 1}}),                                 /* joint at point (1, 1) */
      MakeJoint({{2, 1}}),                                 /* joint at point (1, 2) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// This test checks that having a Only restriction from F0 to F2 it's still possible std::move
// from F0 to F1.
UNIT_CLASS_TEST(RestrictionTest, FGraph_RestrictionF0F2Only)
{
  Init(BuildFGraph());
  RestrictionVec restrictionsNo;
  RestrictionVec restrictionsOnly = {{0 /* feature from */, 2 /* feature to */}};
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {0, 1}, {1, 1}};

  TestRestrictions(expectedGeom, Algorithm::Result::OK,
                   MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
                   MakeFakeEnding(1, 0, m2::PointD(1, 1), *m_graph), std::move(restrictionsNo), *this);
}

/// @todo By VNG: no-pass-through behaviour was changed.
/// @see IndexGraphStarter::CheckLength
//                  *---F4---*
//                  |        |
//                 F3        F5
//                  |        |
// 0 Start *---F0---*---F1---*---F2---* Finish
//         0        1        2        3
unique_ptr<SingleVehicleWorldGraph> BuildNonPassThroughGraph(bool passThroughStart, bool passThroughShortWay,
                                                             bool passThroughLongWay)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->SetPassThroughAllowed(0 /* feature id */, passThroughStart);
  loader->AddRoad(1 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}}));
  loader->SetPassThroughAllowed(1 /* feature id */, passThroughShortWay);
  loader->AddRoad(2 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 1.0}}));
  loader->SetPassThroughAllowed(4 /* feature id */, passThroughLongWay);
  loader->AddRoad(5 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 1.0}, {2.0, 0.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {1, 0}, {3, 0}}),                 /* joint at point (1, 0) */
      MakeJoint({{1, 1}, {2, 0}, {5, 1}}),                 /* joint at point (2, 0) */
      MakeJoint({{3, 1}, {4, 0}}),                         /* joint at point (1, 1) */
      MakeJoint({{4, 1}, {5, 0}}),                         /* joint at point (2, 1) */
      MakeJoint({{2, 1}}),                                 /* joint at point (3, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, NonPassThroughStart)
{
  Init(BuildNonPassThroughGraph(false /* passThroughStart */, true /* passThroughShortWay */,
                                true /* passThroughLongWay */));

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));

  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}};
  TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, NonPassThroughShortWay)
{
  Init(BuildNonPassThroughGraph(true /* passThroughStart */, false /* passThroughShortWay */,
                                true /* passThroughLongWay */));

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));

  // vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {2, 1}, {2, 0}, {3, 0}};
  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}};
  TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, NonPassThroughWay)
{
  Init(BuildNonPassThroughGraph(true /* passThroughStart */, false /* passThroughShortWay */,
                                false /* passThroughLongWay */));

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));

  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}};
  TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, NontransiStartAndShortWay)
{
  Init(BuildNonPassThroughGraph(false /* passThroughStart */, false /* passThroughShortWay */,
                                true /* passThroughLongWay */));
  // We can get F1 because F0 is in the same non-pass-through area/
  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}};

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));
  TestRouteGeometry(*m_starter, Algorithm::Result::OK, expectedGeom);
}

// 2                 *
//                ↗     ↘
//              F5        F4
//            ↗              ↘                  Finish
// 1         *                 *<- F3 ->*-> F8 -> *
//            ↖                         ↑
//              F6                      F2
//         Start   ↖                    ↑
// 0         *-> F7 ->*-> F0 ->*-> F1 ->*
//          -1        0        1        2         3
//
unique_ptr<SingleVehicleWorldGraph> BuildTwoCubeGraph1()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(1 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {2.0, 1.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {1.0, 1.0}}));
  loader->AddRoad(5 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(6 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {-1.0, 1.0}}));
  loader->AddRoad(7 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(8 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 1.0}, {3.0, 1.0}}));

  vector<Joint> const joints = {
      // {{/* feature id */, /* point id */}, ... }
      MakeJoint({{7, 0}}),                 /* joint at point (-1, 0) */
      MakeJoint({{0, 0}, {6, 0}, {7, 1}}), /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {1, 0}}),         /* joint at point (1, 0) */
      MakeJoint({{1, 1}, {2, 0}}),         /* joint at point (2, 0) */
      MakeJoint({{2, 1}, {3, 1}, {8, 0}}), /* joint at point (2, 1) */
      MakeJoint({{3, 0}, {4, 1}}),         /* joint at point (1, 1) */
      MakeJoint({{5, 1}, {4, 0}}),         /* joint at point (0, 2) */
      MakeJoint({{6, 1}, {5, 0}}),         /* joint at point (-1, 1) */
      MakeJoint({{8, 1}}),                 /* joint at point (3, 1) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

// 2                 *
//                ↗     ↘
//              F5        F4
//            ↗              ↘                             Finish
// 1         *                 *<- F3 ->*-> F8 -> *-> F10 -> *
//            ↖                         ↑       ↗
//              F6                      F2   F9
//         Start   ↖                    ↑  ↗
// 0         *-> F7 ->*-> F0 ->*-> F1 ->*
//          -1        0        1        2         3          4
//
unique_ptr<SingleVehicleWorldGraph> BuildTwoCubeGraph2()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(1 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {2.0, 1.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {1.0, 1.0}}));
  loader->AddRoad(5 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(6 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {-1.0, 1.0}}));
  loader->AddRoad(7 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(8 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 1.0}, {3.0, 1.0}}));
  loader->AddRoad(9 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 1.0}}));
  loader->AddRoad(10 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 1.0}, {4.0, 1.0}}));

  vector<Joint> const joints = {
      // {{/* feature id */, /* point id */}, ... }
      MakeJoint({{7, 0}}),                  /* joint at point (-1, 0) */
      MakeJoint({{0, 0}, {6, 0}, {7, 1}}),  /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {1, 0}}),          /* joint at point (1, 0) */
      MakeJoint({{1, 1}, {2, 0}, {9, 0}}),  /* joint at point (2, 0) */
      MakeJoint({{2, 1}, {3, 1}, {8, 0}}),  /* joint at point (2, 1) */
      MakeJoint({{3, 0}, {4, 1}}),          /* joint at point (1, 1) */
      MakeJoint({{5, 1}, {4, 0}}),          /* joint at point (0, 2) */
      MakeJoint({{6, 1}, {5, 0}}),          /* joint at point (-1, 1) */
      MakeJoint({{8, 1}, {9, 1}, {10, 0}}), /* joint at point (3, 1) */
      MakeJoint({{10, 1}})                  /* joint at point (4, 1) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(std::move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, RestrictionNoWithWayAsVia_1)
{
  Init(BuildTwoCubeGraph1());

  m2::PointD const start(-1.0, 0.0);
  m2::PointD const finish(3.0, 1.0);
  auto const test = [&](vector<m2::PointD> const & expectedGeom, RestrictionVec && restrictionsNo)
  {
    TestRestrictions(
        expectedGeom, Algorithm::Result::OK, MakeFakeEnding(7 /* featureId */, 0 /* segmentIdx */, start, *m_graph),
        MakeFakeEnding(8 /* featureId */, 0 /* segmentIdx */, finish, *m_graph), std::move(restrictionsNo), *this);
  };

  // Can not go from |0| to |2| via |1|
  RestrictionVec restrictionsNo = {{0 /* feature 0 */, 1 /* feature 1 */, 2 /* feature 2 */}};

  // Check that without restrictions we can find path better.
  test({start, {0.0, 0.0}, {-1.0, 1.0}, {0.0, 2.0}, {1.0, 1.0}, {2.0, 1.0}, finish}, std::move(restrictionsNo));

  test({start, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, finish}, RestrictionVec());
}

UNIT_CLASS_TEST(RestrictionTest, RestrictionNoWithWayAsVia_2)
{
  Init(BuildTwoCubeGraph2());

  m2::PointD const start(-1.0, 0.0);
  m2::PointD const finish(4.0, 1.0);
  auto const test = [&](vector<m2::PointD> const & expectedGeom, RestrictionVec && restrictionsNo)
  {
    TestRestrictions(
        expectedGeom, Algorithm::Result::OK, MakeFakeEnding(7 /* featureId */, 0 /* segmentIdx */, start, *m_graph),
        MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, finish, *m_graph), std::move(restrictionsNo), *this);
  };

  // Can go from |0| to |9| only via |1|
  RestrictionVec restrictionsNo = {{0 /* feature 0 */, 1 /* feature 1 */, 9 /* feature 2 */}};

  // Check that without restrictions we can find path better.
  test({start, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {3.0, 1.0}, finish}, std::move(restrictionsNo));

  test({start, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 1.0}, finish}, RestrictionVec());
}

UNIT_CLASS_TEST(RestrictionTest, RestrictionOnlyWithWayAsVia_1)
{
  Init(BuildTwoCubeGraph2());

  m2::PointD const start(-1.0, 0.0);
  m2::PointD const finish(4.0, 1.0);
  auto const test = [&](vector<m2::PointD> const & expectedGeom, RestrictionVec && restrictionsNo)
  {
    TestRestrictions(
        expectedGeom, Algorithm::Result::OK, MakeFakeEnding(7 /* featureId */, 0 /* segmentIdx */, start, *m_graph),
        MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, finish, *m_graph), std::move(restrictionsNo), *this);
  };

  RestrictionVec restrictionsNo;
  // Can go from |0| to |2| only via |1|
  RestrictionVec restrictionsOnly = {{0 /* feature 0 */, 1 /* feature 1 */, 2 /* feature 2 */}};
  ConvertRestrictionsOnlyToNo(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly, restrictionsNo);

  // Check that without restrictions we can find path better.
  test({start, {0, 0}, {1, 0}, {2, 0}, {2, 1}, {3, 1}, finish}, std::move(restrictionsNo));
  test({start, {0, 0}, {1, 0}, {2, 0}, {3, 1}, finish}, RestrictionVec());
}
}  // namespace restriction_test
