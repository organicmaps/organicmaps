#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/fake_ending.hpp"
#include "routing/geometry.hpp"
#include "routing/restriction_loader.hpp"

#include "geometry/point2d.hpp"

#include <memory>
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
// Start * -- F0 --> * -- F1 --> * <-- F2 --> * -- F3 --> *
//                              | ^
//                              | |
//                             F4 F5
//                              | |
//                              ⌄ |
//                               *
unique_ptr<SingleVehicleWorldGraph> BuildCrossGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(1 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(2 /* featureId */, false /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.9999, 0.0}}));
  loader->AddRoad(3 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.9999, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, -1.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, -1.0}, {1.0, 0.0}}));
  loader->AddRoad(6 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(7 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {1.0, 2.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0, 1}, {1, 0}}), MakeJoint({{1, 1}, {2, 0}, {4, 0}, {5, 1}, {6, 0}}),
      MakeJoint({{2, 1}, {3, 0}}), MakeJoint({{4, 1}, {5, 0}}), MakeJoint({{6, 1}, {7, 0}})};

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, CrossGraph_NoUTurn)
{
  Init(BuildCrossGraph());
  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *m_graph),
             MakeFakeEnding(7, 0, m2::PointD(1, 2), *m_graph));

  vector<m2::PointD> const expectedGeom = {
      {-1.0 /* x */, 0.0 /* y */}, {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}};
  TestRouteGeometry(*m_starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, CrossGraph_UTurn)
{
  Init(BuildCrossGraph());
  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *m_graph),
             MakeFakeEnding(7, 0, m2::PointD(1, 2), *m_graph));

  RestrictionVec restrictions = {
      {Restriction::Type::No, {1 /* feature from */, 6 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {{-1.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {1.0, -1.0},
                                           {1.0, 0.0},  {1.0, 1.0}, {1.0, 2.0}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *m_graph),
      MakeFakeEnding(7, 0, m2::PointD(1, 2), *m_graph), move(restrictions), *this);
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

// Route through triangular graph without any restrictions.
UNIT_CLASS_TEST(RestrictionTest, TriangularGraph)
{
  Init(BuildTriangularGraph());

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), {}, *this);
}

// Route through triangular graph with restriction type no from feature 2 to feature 1.
UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionNoF2F1)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {2 /* feature from */, 1 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(5 /* featureId */, 0 /* senmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), move(restrictions), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionNoF5F2)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {5 /* feature from */, 2 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), move(restrictions), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionOnlyF5F3)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictionsOnly = {
      {Restriction::Type::Only, {5 /* feature from */, 3 /* feature to */}}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNoAndSort(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly,
                                     restrictionsNo);
  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TriangularGraph_RestrictionNoF5F2RestrictionOnlyF5F3)
{
  Init(BuildTriangularGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {5 /* feature from */, 2 /* feature to */}},
      {Restriction::Type::Only, {5 /* feature from */, 3 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(5 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), move(restrictions), *this);
}

// Finish
// 3 *
//   |
//   F4
//   |
// 2 *
//   | \
//   F0  F2
//   |     \
// 1 *       *
//   |         \
//   F0         F2
//   |             \
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, TwowayCornerGraph)
{
  Init(BuildTwowayCornerGraph());
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), {}, *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwowayCornerGraph_RestrictionF3F2No)
{
  Init(BuildTwowayCornerGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {3 /* feature from */, 2 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), move(restrictions), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwowayCornerGraph_RestrictionF3F1Only)
{
  Init(BuildTwowayCornerGraph());
  RestrictionVec restrictionsOnly = {
      {Restriction::Type::Only, {3 /* feature from */, 1 /* feature to */}}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNoAndSort(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly,
                                     restrictionsNo);
  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(4, 0, m2::PointD(0, 3), *m_graph), move(restrictionsNo), *this);
}

// Finish
// 3 *
//   ^
//   |
//  F11
//   |
// 2 *<---F5----*<---F6---*
//   ^ ↖       ^ ↖       ^
//   |   F7     |   F8    |
//   |     ↖   F1    ↖   F2
//   |       ↖ |       ↖ |
// 1 F0         *          *
//   |          ^  ↖      ^
//   |         F1     F9  F2
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph)
{
  Init(BuildTwoSquaresGraph());
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), {}, *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph_RestrictionF10F3Only)
{
  Init(BuildTwoSquaresGraph());
  RestrictionVec restrictionsOnly = {
      {Restriction::Type::Only, {10 /* feature from */, 3 /* feature to */}}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNoAndSort(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly,
                                     restrictionsNo);

  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph_RestrictionF10F3OnlyF3F4Only)
{
  Init(BuildTwoSquaresGraph());
  RestrictionVec restrictionsOnly = {
      {Restriction::Type::Only, {3 /* feature from */, 4 /* feature to */}},
      {Restriction::Type::Only, {10 /* feature from */, 3 /* feature to */}}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNoAndSort(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly,
                                     restrictionsNo);

  vector<m2::PointD> const expectedGeom = {
      {3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), move(restrictionsNo), *this);
}

UNIT_CLASS_TEST(RestrictionTest, TwoSquaresGraph_RestrictionF2F8NoRestrictionF9F1Only)
{
  Init(BuildTwoSquaresGraph());
  RestrictionVec restrictionsNo = {
      {Restriction::Type::No, {2 /* feature from */, 8 /* feature to */}}};  // Invalid restriction.
  RestrictionVec const restrictionsOnly = {
      {Restriction::Type::Only,
       {9 /* feature from */, 1 /* feature to */}}};  // Invalid restriction.
  ConvertRestrictionsOnlyToNoAndSort(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly,
                                     restrictionsNo);

  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 1}, {0, 2}, {0, 3}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(10 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(11, 0, m2::PointD(0, 3), *m_graph), move(restrictionsNo), *this);
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
// Note 1. All features are two-way. (It's possible to move along any direction of the features.)
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

// Route through flag graph without any restrictions.
UNIT_TEST(FlagGraph)
{
  unique_ptr<WorldGraph> graph = BuildFlagGraph();
  auto starter =
      MakeStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *graph),
                  MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {0.5, 1}};
  TestRouteGeometry(*starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through flag graph with one restriciton (type no) from F0 to F3.
UNIT_CLASS_TEST(RestrictionTest, FlagGraph_RestrictionF0F3No)
{
  Init(BuildFlagGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {0 /* feature from */, 3 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
      MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *m_graph), move(restrictions), *this);
}

// Route through flag graph with one restriciton (type only) from F0 to F1.
UNIT_CLASS_TEST(RestrictionTest, FlagGraph_RestrictionF0F1Only)
{
  Init(BuildFlagGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {0 /* feature from */, 1 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {0.5, 1}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
      MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *m_graph), move(restrictions), *this);
}

UNIT_CLASS_TEST(RestrictionTest, FlagGraph_PermutationsF1F3NoF7F8OnlyF8F4OnlyF4F6Only)
{
  Init(BuildFlagGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {0 /* feature from */, 3 /* feature to */}},
      {Restriction::Type::Only, {0 /* feature from */, 1 /* feature to */}},
      {Restriction::Type::Only, {1 /* feature from */, 2 /* feature to */}}};

  vector<m2::PointD> const expectedGeom = {
      {2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}};
  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
      MakeFakeEnding(6, 0, m2::PointD(0.5, 1), *m_graph), move(restrictions), *this);
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

// Route through poster graph without any restrictions.
UNIT_TEST(PosterGraph)
{
  unique_ptr<WorldGraph> graph = BuildPosterGraph();
  auto starter =
      MakeStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *graph),
                  MakeFakeEnding(6, 0, m2::PointD(2, 1), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {2, 1}};

  TestRouteGeometry(*starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through poster graph with restrictions F0-F3 (type no).
UNIT_CLASS_TEST(RestrictionTest, PosterGraph_RestrictionF0F3No)
{
  Init(BuildPosterGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {0 /* feature from */, 3 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}, {1, 1}, {2, 1}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
      MakeFakeEnding(6, 0, m2::PointD(2, 1), *m_graph), move(restrictions), *this);
}

// Route through poster graph with restrictions F0-F1 (type only).
UNIT_CLASS_TEST(RestrictionTest, PosterGraph_RestrictionF0F1Only)
{
  Init(BuildPosterGraph());

  RestrictionVec restrictionsOnly = {
      {Restriction::Type::Only, {0 /* feature from */, 1 /* feature to */}}};
  RestrictionVec restrictionsNo;
  ConvertRestrictionsOnlyToNoAndSort(m_graph->GetIndexGraphForTests(kTestNumMwmId), restrictionsOnly,
                                     restrictionsNo);

  vector<m2::PointD> const expectedGeom = {
      {2 /* x */, 0 /* y */}, {1, 0}, {0, 0}, {0, 1}, {0.5, 1}, {1, 1}, {2, 1}};
  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(2, 0), *m_graph),
      MakeFakeEnding(6, 0, m2::PointD(2, 1), *m_graph), move(restrictionsNo), *this);
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
      MakeJoint(
          {{0 /* feature id */, 0 /* point id */}, {1, 0}, {3, 1}}), /* joint at point (0, 0) */
      MakeJoint(
          {{0 /* feature id */, 2 /* point id */}, {1, 3}, {2, 0}}), /* joint at point (3, 0) */
      MakeJoint({{3 /* feature id */, 0 /* point id */}}),           /* joint at point (-1, 0) */
      MakeJoint({{2 /* feature id */, 1 /* point id */}}),           /* joint at point (4, 0) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);
  return BuildWorldGraph(move(loader), estimator, joints);
}

UNIT_TEST(TwoWayGraph)
{
  unique_ptr<WorldGraph> graph = BuildTwoWayGraph();
  auto starter =
      MakeStarter(MakeFakeEnding(3 /* featureId */, 0 /* segmentIdx */, m2::PointD(-1, 0), *graph),
                  MakeFakeEnding(2, 0, m2::PointD(4, 0), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{-1 /* x */, 0 /* y */}, {0, 0}, {1, 0}, {3, 0}, {4, 0}};

  TestRouteGeometry(*starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// 1          *---F4----*
//            |         |
//           F2        F3
//            |         |
// 0 *<--F5---*<--F1----*<--F0---* Start
// Finish
//   0        1        2         3
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

UNIT_TEST(SquaresGraph)
{
  unique_ptr<WorldGraph> graph = BuildSquaresGraph();
  auto starter =
      MakeStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *graph),
                  MakeFakeEnding(5, 0, m2::PointD(0, 0), *graph), *graph);
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}};
  TestRouteGeometry(*starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// It's a test on correct working in case when because of adding restrictions
// start and finish could be match on blocked, moved or copied edges.
// See IndexGraphStarter constructor for a detailed description.
UNIT_CLASS_TEST(RestrictionTest, SquaresGraph_RestrictionF0F1OnlyF1F5Only)
{
  Init(BuildSquaresGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::Only, {0 /* feature from */, 1 /* feature to */}},
      {Restriction::Type::Only, {1 /* feature from */, 5 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {{3 /* x */, 0 /* y */}, {2, 0}, {1, 0}, {0, 0}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(3, 0), *m_graph),
      MakeFakeEnding(5, 0, m2::PointD(0, 0), *m_graph), move(restrictions), *this);
}

// 0 Start *--F0--->*---F1---*---F1---*---F1---*---F2-->* Finish
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

// This test checks that despite the fact uturn on F1 is prohibited (moving from F1 to F1 is
// prohibited)
// it's still possible to move from F1 to F1 in straight direction.
UNIT_CLASS_TEST(RestrictionTest, LineGraph_RestrictionF1F1No)
{
  Init(BuildLineGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::No, {1 /* feature from */, 1 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {
      {0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
      MakeFakeEnding(2, 0, m2::PointD(5, 0), *m_graph), move(restrictions), *this);
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

// This test checks that having a Only restriction from F0 to F2 it's still possible move
// from F0 to F1.
UNIT_CLASS_TEST(RestrictionTest, FGraph_RestrictionF0F2Only)
{
  Init(BuildFGraph());
  RestrictionVec restrictions = {
      {Restriction::Type::Only, {0 /* feature from */, 2 /* feature to */}}};
  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {0, 1}, {1, 1}};

  TestRestrictions(
      expectedGeom, AStarAlgorithm<IndexGraphStarter>::Result::OK,
      MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
      MakeFakeEnding(1, 0, m2::PointD(1, 1), *m_graph), move(restrictions), *this);
}

//                  *---F4---*
//                  |        |
//                 F3        F5
//                  |        |
// 0 Start *---F0---*---F1---*---F2---* Finish
//         0        1        2        3
unique_ptr<SingleVehicleWorldGraph> BuildNontransitGraph(bool transitStart, bool transitShortWay,
                                                         bool transitLongWay)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->SetTransitAllowed(0 /* feature id */, transitStart);
  loader->AddRoad(1 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}}));
  loader->SetTransitAllowed(1 /* feature id */, transitShortWay);
  loader->AddRoad(2 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 1.0}}));
  loader->SetTransitAllowed(4 /* feature id */, transitLongWay);
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
  return BuildWorldGraph(move(loader), estimator, joints);
}

UNIT_CLASS_TEST(RestrictionTest, NontransitStart)
{
  Init(BuildNontransitGraph(false /* transitStart */, true /* transitShortWay */,
                            true /* transitLongWay */));
  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}};

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));
  TestRouteGeometry(*m_starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, NontransitShortWay)
{
  Init(BuildNontransitGraph(true /* transitStart */, false /* transitShortWay */,
                            true /* transitLongWay */));
  vector<m2::PointD> const expectedGeom = {
      {0 /* x */, 0 /* y */}, {1, 0}, {1, 1}, {2, 1}, {2, 0}, {3, 0}};

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));
  TestRouteGeometry(*m_starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

UNIT_CLASS_TEST(RestrictionTest, NontransitWay)
{
  Init(BuildNontransitGraph(true /* transitStart */, false /* transitShortWay */,
                            false /* transitLongWay */));

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));
  TestRouteGeometry(*m_starter, AStarAlgorithm<IndexGraphStarter>::Result::NoPath, {});
}

UNIT_CLASS_TEST(RestrictionTest, NontransiStartAndShortWay)
{
  Init(BuildNontransitGraph(false /* transitStart */, false /* transitShortWay */,
                            true /* transitLongWay */));
  // We can get F1 because F0 is in the same nontransit area/
  vector<m2::PointD> const expectedGeom = {{0 /* x */, 0 /* y */}, {1, 0}, {2, 0}, {3, 0}};

  SetStarter(MakeFakeEnding(0 /* featureId */, 0 /* segmentIdx */, m2::PointD(0, 0), *m_graph),
             MakeFakeEnding(2, 0, m2::PointD(3, 0), *m_graph));
  TestRouteGeometry(*m_starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}
}  // namespace routing_test
