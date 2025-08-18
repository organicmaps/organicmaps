#include "testing/testing.hpp"

#include "routing/restriction_loader.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"
#include "routing/routing_tests/world_graph_builder.hpp"

namespace uturn_restriction_tests
{
using namespace routing;
using namespace routing_test;

using Algorithm = AStarAlgorithm<Segment, SegmentEdge, RouteWeight>;

UNIT_CLASS_TEST(NoUTurnRestrictionTest, CheckNoUTurn_1)
{
  Init(BuildCrossGraph());

  Segment const start(kTestNumMwmId, 3 /* fId */, 0 /* segId */, true /* forward */);
  Segment const finish(kTestNumMwmId, 7 /* fId */, 0 /* segId */, true /* forward */);

  std::vector<m2::PointD> const expectedGeom = {{3.0, 0.0}, {2.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}};

  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeom);

  std::vector<RestrictionUTurn> noUTurnRestrictions = {{3 /* featureId */, false /* viaIsFirstPoint */}};

  SetNoUTurnRestrictions(std::move(noUTurnRestrictions));
  TestRouteGeom(start, finish, Algorithm::Result::NoPath, {} /* expectedGeom */);
}

UNIT_CLASS_TEST(NoUTurnRestrictionTest, CheckNoUTurn_2)
{
  Init(BuildCrossGraph());

  Segment const start(kTestNumMwmId, 2 /* fId */, 0 /* segId */, true /* forward */);
  Segment const finish(kTestNumMwmId, 7 /* fId */, 0 /* segId */, true /* forward */);

  std::vector<m2::PointD> const expectedGeom = {{2.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}};

  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeom);

  std::vector<RestrictionUTurn> noUTurnRestrictions = {{2 /* featureId */, false /* viaIsFirstPoint */}};

  std::vector<m2::PointD> const expectedGeomAfterRestriction = {{2.0, 0.0}, {3.0, 0.0}, {2.0, 0.0},
                                                                {1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}};

  SetNoUTurnRestrictions(std::move(noUTurnRestrictions));
  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeomAfterRestriction);
}

UNIT_CLASS_TEST(NoUTurnRestrictionTest, CheckNoUTurn_3)
{
  Init(BuildCrossGraph());

  Segment const start(kTestNumMwmId, 2 /* fId */, 0 /* segId */, false /* forward */);
  Segment const finish(kTestNumMwmId, 3 /* fId */, 0 /* segId */, true /* forward */);

  std::vector<m2::PointD> const expectedGeom = {{1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}};

  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeom);

  std::vector<RestrictionUTurn> noUTurnRestrictions = {{2 /* featureId */, true /* viaIsFirstPoint */}};

  std::vector<m2::PointD> const expectedGeomAfterRestriction = {
      {1.0, 0.0}, {1.0, -1.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}};

  SetNoUTurnRestrictions(std::move(noUTurnRestrictions));
  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeomAfterRestriction);
}

UNIT_CLASS_TEST(NoUTurnRestrictionTest, CheckOnlyUTurn_1)
{
  Init(BuildCrossGraph());

  Segment const start(kTestNumMwmId, 2 /* fId */, 0 /* segId */, true /* forward */);
  Segment const finish(kTestNumMwmId, 3 /* fId */, 0 /* segId */, true /* forward */);

  std::vector<m2::PointD> const expectedGeom = {{2.0, 0.0}, {3.0, 0.0}};

  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeom);

  std::vector<RestrictionUTurn> onlyUTurnRestrictions = {{2 /* featureId */, false /* viaIsFirstPoint */}};

  RestrictionVec restrictionsNo;

  auto & indexGraph = m_graph->GetWorldGraph().GetIndexGraph(kTestNumMwmId);
  ConvertRestrictionsOnlyUTurnToNo(indexGraph, onlyUTurnRestrictions, restrictionsNo);
  SetRestrictions(std::move(restrictionsNo));

  TestRouteGeom(start, finish, Algorithm::Result::NoPath, {} /* expectedGeom */);
}

UNIT_CLASS_TEST(NoUTurnRestrictionTest, CheckOnlyUTurn_2)
{
  Init(BuildTestGraph());

  Segment const start(kTestNumMwmId, 0 /* fId */, 0 /* segId */, false /* forward */);
  Segment const finish(kTestNumMwmId, 5 /* fId */, 0 /* segId */, true /* forward */);

  std::vector<m2::PointD> const expectedGeom = {{0.0, 1.0}, {1.0, 1.0}, {2.0, 2.0}, {3.0, 2.0}};

  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeom);

  std::vector<RestrictionUTurn> onlyUTurnRestrictions = {{1 /* featureId */, false /* viaIsFirstPoint */}};

  RestrictionVec restrictionsNo;

  auto & indexGraph = m_graph->GetWorldGraph().GetIndexGraph(kTestNumMwmId);
  ConvertRestrictionsOnlyUTurnToNo(indexGraph, onlyUTurnRestrictions, restrictionsNo);
  SetRestrictions(std::move(restrictionsNo));

  std::vector<m2::PointD> const expectedGeomAfterRestriction = {{0.0, 1.0}, {-1.0, 1.0}, {-1.0, 2.0}, {0.0, 2.0},
                                                                {1.0, 2.0}, {2.0, 2.0},  {3.0, 2.0}};

  TestRouteGeom(start, finish, Algorithm::Result::OK, expectedGeomAfterRestriction);
}
}  // namespace uturn_restriction_tests
