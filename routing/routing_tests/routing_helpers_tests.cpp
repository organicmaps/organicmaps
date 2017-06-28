#include "testing/testing.hpp"

#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/segment.hpp"
#include "routing/turns.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace routing_test
{
using namespace routing::turns;
using namespace routing;
using namespace std;

UNIT_TEST(FillSegmentInfoSmokeTest)
{
  vector<Segment> const segments = {
      {0 /* mwmId */, 1 /* featureId */, 0 /* segmentIdx */, true /* forward */}};
  vector<Junction> const junctions = {
      {m2::PointD(0.0 /* x */, 0.0 /* y */), feature::kInvalidAltitude},
      {m2::PointD(0.1 /* x */, 0.0 /* y */), feature::kInvalidAltitude}};
  Route::TTurns const & turnDirs = {{1 /* point index */, TurnDirection::ReachedYourDestination}};
  Route::TStreets const streets = {};
  Route::TTimes const times = {{0 /* point index */, 0.0 /* time in seconds */}, {1, 1.0}};

  vector<Route::SegmentInfo> segmentInfo;
  FillSegmentInfo(segments, junctions, turnDirs, streets, times, nullptr, segmentInfo);

  TEST_EQUAL(segmentInfo.size(), 1, ());
  TEST_EQUAL(segmentInfo[0].m_turn.m_turn, TurnDirection::ReachedYourDestination, ());
  TEST(segmentInfo[0].m_street.empty(), ());
}

UNIT_TEST(FillSegmentInfoTest)
{
  vector<Segment> const segments = {
      {0 /* mwmId */, 1 /* featureId */, 0 /* segmentIdx */, true /* forward */}, {0, 2, 0, true}};
  vector<Junction> const junctions = {
      {m2::PointD(0.0 /* x */, 0.0 /* y */), feature::kInvalidAltitude},
      {m2::PointD(0.1 /* x */, 0.0 /* y */), feature::kInvalidAltitude},
      {m2::PointD(0.2 /* x */, 0.0 /* y */), feature::kInvalidAltitude}};
  Route::TTurns const & turnDirs = {{1 /* point index */, TurnDirection::TurnRight},
                                    {2 /* point index */, TurnDirection::ReachedYourDestination}};
  Route::TStreets const streets = {{0 /* point index */, "zero"}, {1, "first"}, {2, "second"}};
  Route::TTimes const times = {
      {0 /* point index */, 0.0 /* time in seconds */}, {1, 1.0}, {2, 2.0}};

  vector<Route::SegmentInfo> segmentInfo;
  FillSegmentInfo(segments, junctions, turnDirs, streets, times, nullptr, segmentInfo);

  TEST_EQUAL(segmentInfo.size(), 2, ());
  TEST_EQUAL(segmentInfo[0].m_turn.m_turn, TurnDirection::TurnRight, ());
  TEST_EQUAL(segmentInfo[0].m_street, string("first"), ());
  TEST_EQUAL(segmentInfo[0].m_segment, segments[0], ());

  TEST_EQUAL(segmentInfo[1].m_turn.m_turn, TurnDirection::ReachedYourDestination, ());
  TEST_EQUAL(segmentInfo[1].m_street, string("second"), ());
  TEST_EQUAL(segmentInfo[1].m_segment, segments[1], ());
}
}  // namespace routing_test