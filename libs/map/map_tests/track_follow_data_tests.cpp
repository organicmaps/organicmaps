#include "testing/testing.hpp"

#include "map/track.hpp"
#include "map/track_follow_data.hpp"

#include "routing/route.hpp"
#include "routing/track_router.hpp"

#include "geometry/mercator.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace track_follow_data_tests
{
namespace
{
m2::PointD MeterPoint(double x, double y)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

Track MakeSourceTrack(kml::TrackData data)
{
  data.m_id = 1;
  return Track(std::move(data));
}
}  // namespace

UNIT_TEST(TrackFollowData_TrackEntryResolvesBothSidesOfGap)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 110}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(500, 0), 120}, {MeterPoint(600, 0), 130}});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);
  double const boundary = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0));
  auto const before = ResolveTrackEntry(track, MeterPoint(100, 0), boundary);
  auto const after = ResolveTrackEntry(track, MeterPoint(500, 0), boundary);
  TEST(before && after, ());
  TEST_EQUAL(before->m_line, 0, ());
  TEST_EQUAL(after->m_line, 1, ());
  TEST(ResolveTrackEntryAt(track, boundary) == *before, ("Chart distances select the endpoint preceding a gap"));
  auto const forward = BuildTrackFollowingData(source, *after);
  TEST_EQUAL(forward->m_points.front().GetPoint(), MeterPoint(500, 0), ());
  TEST_EQUAL(forward->m_points.back().GetPoint(), MeterPoint(600, 0), ());
  TEST(!ResolveTrackEntry(track, MeterPoint(500, 0), 0.0), ("A stale distance must not select a new occurrence"));
}

UNIT_TEST(TrackFollowData_InvalidTrackSelectionDistance)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);
  TEST(!ResolveTrackEntry(track, MeterPoint(0, 0), -0.005), ("Reject negatives within the match tolerance"));
  TEST(ResolveTrackEntry(track, MeterPoint(0, 0), 0.0), ());
}

UNIT_TEST(TrackFollowData_ChartEndAfterShortFinalSegment)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(1000000, 0), 110}, {MeterPoint(1000001, 0), 120}});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);
  auto const end = ResolveTrackEntryAt(track, track.GetLengthMeters());
  TEST(end == (TrackSourcePosition{0, 1, 1.0}), (end.m_line, end.m_segment, end.m_fraction));
  TEST(!HasRemainingTrackGeometry(source.m_geometry, end), ());
}

UNIT_TEST(TrackFollowData_TrackPreparationKeepsSelectedStopAndHill)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(400, 0), 250}, {MeterPoint(1000, 0), 100}});
  source.m_geometry.AddTimestamps({100, 200, 300});
  source.m_stops = {{MeterPoint(100, 0), "Before"}, {MeterPoint(400, 0), "Entry"}, {MeterPoint(600, 0), "After"}};
  auto const original = source;
  auto const entry = ResolveTrackEntry(MakeSourceTrack(source), MeterPoint(400, 0),
                                       mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0)));
  TEST(entry, ());
  auto const data = BuildTrackFollowingData(source, *entry);
  TEST_EQUAL(data->m_points.size(), 3, ());
  TEST_EQUAL(data->m_points[0].GetAltitude(), 250, ());
  TEST_EQUAL(data->m_points[1].GetPoint(), MeterPoint(600, 0), ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({0, 1}), ());
  TEST_EQUAL(source, original, ());
}

UNIT_TEST(TrackFollowData_FractionalTrackEntryPreservesAltitudesAndStops)
{
  struct TestCase
  {
    geometry::Altitude m_from;
    geometry::Altitude m_to;
    geometry::Altitude m_entry;
  };
  auto constexpr kUnknown = geometry::kInvalidAltitude;
  TestCase const cases[] = {{100, 200, 150}, {kUnknown, 200, kUnknown}, {100, kUnknown, kUnknown}};
  for (auto const & test : cases)
  {
    kml::TrackData source;
    source.m_geometry.AddLine(
        {{MeterPoint(0, 0), test.m_from}, {MeterPoint(100, 0), test.m_to}, {MeterPoint(200, 0), 300}});
    source.m_geometry.AddTimestamps({});
    source.m_stops = {{MeterPoint(25, 0), "Before"}, {MeterPoint(75, 0), "After"}};
    auto const distance = mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0)) * 0.5;
    auto prepared = BuildTrackFollowingData(source, ResolveTrackEntryAt(MakeSourceTrack(source), distance));
    TEST(prepared, ());
    auto const data = std::make_shared<routing::TrackFollowingData const>(std::move(*prepared));
    TEST(data->m_points.front().GetPoint().EqualDxDy(MeterPoint(50, 0), mercator::kPointEqualityEps), ());
    TEST_EQUAL(data->m_points.front().GetAltitude(), test.m_entry, (test.m_from, test.m_to));
    TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1}), ());
    TEST(data->m_points[1].GetPoint().EqualDxDy(MeterPoint(75, 0), mercator::kPointEqualityEps), ());

    auto const start = MeterPoint(150, 10);
    routing::TrackRouter router(routing::VehicleType::Pedestrian, data);
    routing::RouterDelegate delegate;
    routing::RoutesResult result;
    routing::Checkpoints checkpoints({start, data->m_points[1].GetPoint(), data->m_points.back().GetPoint()});
    TEST_EQUAL(router.CalculateRoute(checkpoints, {}, false, false, delegate, result),
               routing::RouterResultCode::NoError, ());
    routing::Route const route(result.GetActive());
    TEST_EQUAL(route.GetPoly().GetPoint(1), data->m_points.front().GetPoint(), ());
    TEST_EQUAL(route.HaveAltitudes(), test.m_entry != kUnknown, (test.m_from, test.m_to));
  }
}

UNIT_TEST(TrackFollowData_RepeatedStopsPreferLaterSourceVisit)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}, {MeterPoint(0, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(500, 0), "Outbound"}, {MeterPoint(500, 0), "Return"}};
  auto const entry = ResolveTrackEntry(MakeSourceTrack(source), MeterPoint(600, 0),
                                       mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(600, 0)));
  TEST(entry, ());
  auto const forward = BuildTrackFollowingData(source, *entry);
  TEST_EQUAL(forward->m_stopPointIndexes.size(), 1, ());
  TEST_EQUAL(forward->m_points[forward->m_stopPointIndexes.front()].GetPoint(), MeterPoint(500, 0), ());
  TEST_GREATER(forward->m_stopPointIndexes.front(), 1, ("Keep the return visit after the turnaround"));
  source.m_geometry.m_lines[0].pop_back();
  auto const earlier = ResolveTrackEntry(MakeSourceTrack(source), MeterPoint(400, 0),
                                         mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(400, 0)));
  TEST(earlier, ());
  auto const duplicates = BuildTrackFollowingData(source, *earlier);
  TEST_EQUAL(duplicates->m_stopPointIndexes.size(), 2, ());
  TEST_EQUAL(duplicates->m_stopPointIndexes[0], duplicates->m_stopPointIndexes[1], ("Keep true collocated duplicates"));
}

UNIT_TEST(TrackFollowData_RepeatedOffVertexStopsWaitForReturnVisit)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100},
                             {MeterPoint(100, 0), 100},
                             {MeterPoint(101.2, 0), 100},
                             {MeterPoint(104, 0), 100},
                             {MeterPoint(200, 0), 100},
                             {MeterPoint(200, 2), 100},
                             {MeterPoint(0, 2), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(100, 1.9), "Outbound"}, {MeterPoint(100, 1.9), "Return"}};

  auto const full = BuildTrackFollowingData(source, {0, 0, 0.0});
  TEST(full, ());
  TEST_EQUAL(full->m_stopPointIndexes.size(), 2, ());
  TEST(
      full->m_points[full->m_stopPointIndexes[0]].GetPoint().EqualDxDy(MeterPoint(100, 0), mercator::kPointEqualityEps),
      ());
  TEST(
      full->m_points[full->m_stopPointIndexes[1]].GetPoint().EqualDxDy(MeterPoint(100, 2), mercator::kPointEqualityEps),
      ());

  auto const remainder = BuildTrackFollowingData(source, {0, 3, 0.5});
  TEST(remainder, ());
  TEST_EQUAL(remainder->m_stopPointIndexes.size(), 1, ());
  TEST(remainder->m_points[remainder->m_stopPointIndexes.front()].GetPoint().EqualDxDy(MeterPoint(100, 2),
                                                                                       mercator::kPointEqualityEps),
       ());
}

UNIT_TEST(TrackFollowData_TrackPreparationSkipsDistantStops)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(300, 40), "Off the track"}, {MeterPoint(600, 0), "On the track"}};
  auto const data = BuildTrackFollowingData(source, {0, 0, 0.0});
  TEST(data, ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1}), ());
  TEST_EQUAL(data->m_points[1].GetPoint(), MeterPoint(600, 0), ());
}

UNIT_TEST(TrackFollowData_NoisyTrackStopsKeepSourceOrder)
{
  for (bool const separateTurn : {false, true})
  {
    for (double const offset : {1.2, 1.6})
    {
      kml::TrackData source;
      kml::TrackGeometry line = {{MeterPoint(0, 0), 100}, {MeterPoint(1000, 0), 100}};
      if (separateTurn)
        line.emplace_back(MeterPoint(1000, 2), 100);
      line.emplace_back(MeterPoint(0, 2), 100);
      source.m_geometry.m_lines.push_back(std::move(line));
      source.m_geometry.AddTimestamps({});
      source.m_stops = {{MeterPoint(300, offset), "First outbound stop"},
                        {MeterPoint(600, 0.2), "Second outbound stop"}};
      auto const data = BuildTrackFollowingData(source, {0, 0, 0.0});
      TEST(data, (separateTurn, offset));
      TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1, 2}), (separateTurn, offset));
      TEST(data->m_points[1].GetPoint().EqualDxDy(MeterPoint(300, 0), mercator::kPointEqualityEps), ());
      TEST(data->m_points[2].GetPoint().EqualDxDy(MeterPoint(600, 0), mercator::kPointEqualityEps), ());
    }
  }
}

UNIT_TEST(TrackFollowData_TrackVertexStopsKeepTheirSourceVisit)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100},
                             {MeterPoint(500, 0), 100},
                             {MeterPoint(1000, 0), 100},
                             {MeterPoint(600, 2), 100},
                             {MeterPoint(0, 2), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(500, 0), "Outbound vertex"}, {MeterPoint(600, 2), "Return vertex"}};
  auto const data = BuildTrackFollowingData(source, {0, 0, 0.0});
  TEST(data, ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1, 3}), ());
  TEST_EQUAL(data->m_points[1].GetPoint(), MeterPoint(500, 0), ());
  TEST_EQUAL(data->m_points[3].GetPoint(), MeterPoint(600, 2), ());
}

UNIT_TEST(TrackFollowData_TrackStopUsesTheClosestPointOfItsFirstVisit)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 3), 100},
                             {MeterPoint(300, 3), 100},
                             {MeterPoint(304, 0), 100},
                             {MeterPoint(1000, 0), 100},
                             {MeterPoint(300, 1), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(300, 0), "First visit"}};
  auto const data = BuildTrackFollowingData(source, {0, 0, 0.0});
  TEST(data, ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({2}), ());
  TEST(data->m_points[2].GetPoint().EqualDxDy(MeterPoint(301.44, 1.92), mercator::kPointEqualityEps), ());
}

UNIT_TEST(TrackFollowData_NoisyTrackStopDoesNotJumpAcrossAGap)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(100, 2), 100}, {MeterPoint(200, 2), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(99, 1.6), "Before the gap"}};
  auto const data = BuildTrackFollowingData(source, {0, 0, 0.0});
  TEST(data, ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1}), ());
  TEST(data->m_points[1].GetPoint().EqualDxDy(MeterPoint(99, 0), mercator::kPointEqualityEps), ());
}

UNIT_TEST(TrackFollowData_JoinedTrackEntryDoesNotInventAltitudeExcursion)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 50}, {MeterPoint(100, 0), geometry::kInvalidAltitude}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(100, 0), 100}, {MeterPoint(200, 0), 200}});
  source.m_geometry.AddTimestamps({});
  auto const forward = BuildTrackFollowingData(source, {1, 0, 0.0});
  TEST_EQUAL(forward->m_points.size(), 2, ());
  TEST_EQUAL(forward->m_points.front().GetAltitude(), 100, ());
  TEST_EQUAL(forward->m_points.back().GetAltitude(), 200, ());
}
}  // namespace track_follow_data_tests
