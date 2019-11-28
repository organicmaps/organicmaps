#include "testing/testing.hpp"

#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/segment.hpp"
#include "routing/turns.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"

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
  vector<geometry::PointWithAltitude> const junctions = {
      {m2::PointD(0.0 /* x */, 0.0 /* y */), geometry::kInvalidAltitude},
      {m2::PointD(0.1 /* x */, 0.0 /* y */), geometry::kInvalidAltitude}};
  Route::TTurns const & turnDirs = {{1 /* point index */, CarDirection::ReachedYourDestination}};
  Route::TStreets const streets = {};
  Route::TTimes const times = {{0 /* point index */, 0.0 /* time in seconds */}, {1, 1.0}};

  vector<RouteSegment> segmentInfo;
  FillSegmentInfo(segments, junctions, turnDirs, streets, times, nullptr, segmentInfo);

  TEST_EQUAL(segmentInfo.size(), 1, ());
  TEST_EQUAL(segmentInfo[0].GetTurn().m_turn, CarDirection::ReachedYourDestination, ());
  TEST(segmentInfo[0].GetStreet().empty(), ());
}

UNIT_TEST(FillSegmentInfoTest)
{
  vector<Segment> const segments = {
      {0 /* mwmId */, 1 /* featureId */, 0 /* segmentIdx */, true /* forward */}, {0, 2, 0, true}};
  vector<geometry::PointWithAltitude> const junctions = {
      {m2::PointD(0.0 /* x */, 0.0 /* y */), geometry::kInvalidAltitude},
      {m2::PointD(0.1 /* x */, 0.0 /* y */), geometry::kInvalidAltitude},
      {m2::PointD(0.2 /* x */, 0.0 /* y */), geometry::kInvalidAltitude}};
  Route::TTurns const & turnDirs = {{1 /* point index */, CarDirection::TurnRight},
                                    {2 /* point index */, CarDirection::ReachedYourDestination}};
  Route::TStreets const streets = {{0 /* point index */, "zero"}, {1, "first"}, {2, "second"}};
  Route::TTimes const times = {
      {0 /* point index */, 0.0 /* time in seconds */}, {1, 1.0}, {2, 2.0}};

  vector<RouteSegment> segmentInfo;
  FillSegmentInfo(segments, junctions, turnDirs, streets, times, nullptr, segmentInfo);

  TEST_EQUAL(segmentInfo.size(), 2, ());
  TEST_EQUAL(segmentInfo[0].GetTurn().m_turn, CarDirection::TurnRight, ());
  TEST_EQUAL(segmentInfo[0].GetStreet(), string("first"), ());
  TEST_EQUAL(segmentInfo[0].GetSegment(), segments[0], ());

  TEST_EQUAL(segmentInfo[1].GetTurn().m_turn, CarDirection::ReachedYourDestination, ());
  TEST_EQUAL(segmentInfo[1].GetStreet(), string("second"), ());
  TEST_EQUAL(segmentInfo[1].GetSegment(), segments[1], ());
}

UNIT_TEST(PolylineInRectTest)
{
  // Empty polyline.
  TEST(!RectCoversPolyline({}, m2::RectD()), ());
  TEST(!RectCoversPolyline({}, m2::RectD(0.0, 0.0, 2.0, 2.0)), ());

  // One point polyline outside the rect.
  {
    auto const junctions =
        IRoadGraph::PointWithAltitudeVec({{m2::PointD(3.0, 3.0), 0 /* altitude */}});
    TEST(!RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 2.0, 2.0)), ());
  }

  // One point polyline inside the rect.
  {
    auto const junctions =
        IRoadGraph::PointWithAltitudeVec({{m2::PointD(1.0, 1.0), 0 /* altitude */}});
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 2.0, 2.0)), ());
  }

  // One point polyline on the rect border.
  {
    auto const junctions =
        IRoadGraph::PointWithAltitudeVec({{m2::PointD(0.0, 0.0), 0 /* altitude */}});
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 2.0, 2.0)), ());
  }

  // Two point polyline touching the rect border.
  {
    auto const junctions = IRoadGraph::PointWithAltitudeVec({
        {m2::PointD(-1.0, -1.0), 0 /* altitude */},
        {m2::PointD(0.0, 0.0), 0 /* altitude */},
    });
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 2.0, 2.0)), ());
  }

  // Crossing rect by a segment but no polyline points inside the rect.
  {
    auto const junctions = IRoadGraph::PointWithAltitudeVec({
        {m2::PointD(-1.0, -1.0), 0 /* altitude */},
        {m2::PointD(5.0, 5.0), 0 /* altitude */},
    });
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 2.0, 2.0)), ());
  }

  {
    auto const junctions = IRoadGraph::PointWithAltitudeVec({
        {m2::PointD(0.0, 1.0), 0 /* altitude */},
        {m2::PointD(100.0, 2.0), 0 /* altitude */},
    });
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 100.0, 1.0)), ());
  }

  // Crossing a rect very close to a corner.
  {
    auto const junctions = IRoadGraph::PointWithAltitudeVec({
        {m2::PointD(-1.0, 0.0), 0 /* altitude */},
        {m2::PointD(1.0, 1.9), 0 /* altitude */},
    });
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 1.0, 1.0)), ());
  }

  // Three point polyline crossing the rect.
  {
    auto const junctions = IRoadGraph::PointWithAltitudeVec({
        {m2::PointD(0.0, -1.0), 0 /* altitude */},
        {m2::PointD(1.0, 0.01), 0 /* altitude */},
        {m2::PointD(2.0, -1.0), 0 /* altitude */},
    });
    TEST(RectCoversPolyline(junctions, m2::RectD(0.0, 0.0, 1.0, 1.0)), ());
  }
}
}  // namespace routing_test
