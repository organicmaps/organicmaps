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
  TEST(!BuildTrackFollowingData(source, *after, true), ("Never connect across the preceding gap"));
  auto const backward = BuildTrackFollowingData(source, *before, true);
  TEST(backward, ());
  TEST_EQUAL(backward->m_points.front().GetPoint(), MeterPoint(100, 0), ());
  TEST_EQUAL(backward->m_points.back().GetPoint(), MeterPoint(0, 0), ());
  TEST(!ResolveTrackEntry(track, MeterPoint(500, 0), 0.0), ("A stale distance must not select a new occurrence"));
}

UNIT_TEST(TrackFollowData_ChartGapAfterMultiEdgeSection)
{
  auto const point = [](double lon, geometry::Altitude altitude)
  { return geometry::PointWithAltitude(mercator::FromLatLon(0, lon), altitude); };
  kml::TrackData source;
  source.m_geometry.AddLine({point(0, 100), point(0.029, 110)});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({point(1, 120), point(1.007, 150), point(1.014, 140)});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({point(2, 130), point(2.01, 120)});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);

  auto const * elevation = track.GetElevationInfo();
  TEST(elevation, ());
  std::vector<double> chart;
  elevation->ForEachPoint([&chart](double distance, geometry::Altitude) { chart.push_back(distance); });
  std::vector<double> lengths;
  for (auto const & line : track.GetCumulativeLengths())
    lengths.insert(lengths.end(), line.begin(), line.end());
  TEST_EQUAL(chart, lengths, ("The chart and the track must give every point the same distance"));

  // The end of the multi-edge section, shared with the start of the next one.
  auto const gap = ResolveTrackEntryAt(track, chart[4]);
  TEST(gap == (TrackSourcePosition{1, 1, 1.0}), (gap.m_line, gap.m_segment, gap.m_fraction));
  TEST(!HasRemainingTrackGeometry(source.m_geometry, gap), ());
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
  auto const reverse = BuildTrackFollowingData(source, *entry, true);
  TEST_EQUAL(reverse->m_points.front().GetPoint(), MeterPoint(400, 0), ());
  TEST_EQUAL(reverse->m_points.back().GetPoint(), MeterPoint(0, 0), ());
  TEST_EQUAL(reverse->m_points.size(), 3, ());
  TEST_EQUAL(reverse->m_stopPointIndexes, std::vector<size_t>({0, 1}), ());
  TEST_EQUAL(reverse->m_points[1].GetPoint(), MeterPoint(100, 0), ());
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
    auto const entry = ResolveTrackEntryAt(MakeSourceTrack(source), distance);
    for (bool const reverse : {false, true})
    {
      auto prepared = BuildTrackFollowingData(source, entry, reverse);
      TEST(prepared, ());
      auto const data = std::make_shared<routing::TrackFollowingData const>(std::move(*prepared));
      TEST(data->m_points.front().GetPoint().EqualDxDy(MeterPoint(50, 0), mercator::kPointEqualityEps), (reverse));
      TEST_EQUAL(data->m_points.front().GetAltitude(), test.m_entry, (test.m_from, test.m_to, reverse));
      TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1}), (reverse));
      TEST(data->m_points[1].GetPoint().EqualDxDy(MeterPoint(reverse ? 25 : 75, 0), mercator::kPointEqualityEps),
           (reverse));

      auto const start = MeterPoint(150, 10);
      routing::TrackRouter router(routing::VehicleType::Pedestrian, data);
      routing::RouterDelegate delegate;
      routing::RoutesResult result;
      routing::Checkpoints checkpoints({start, data->m_points[1].GetPoint(), data->m_points.back().GetPoint()});
      TEST_EQUAL(router.CalculateRoute(checkpoints, {}, false, false, delegate, result),
                 routing::RouterResultCode::NoError, (reverse));
      routing::Route const route(result.GetActive());
      TEST_EQUAL(route.GetPoly().GetPoint(1), data->m_points.front().GetPoint(), (reverse));
      TEST_EQUAL(route.HaveAltitudes(), test.m_entry != kUnknown, (test.m_from, test.m_to, reverse));
    }
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
  auto const backward = BuildTrackFollowingData(source, *entry, true);
  TEST_EQUAL(backward->m_stopPointIndexes, std::vector<size_t>({1}), ());
  TEST_EQUAL(backward->m_points[1].GetPoint(), MeterPoint(500, 0), ());
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
                             {MeterPoint(600, 0), 100},
                             {MeterPoint(0, 2), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_stops = {{MeterPoint(500, 0), "Outbound vertex"}, {MeterPoint(600, 0), "Return vertex"}};
  auto const data = BuildTrackFollowingData(source, {0, 0, 0.0});
  TEST(data, ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1, 3}), ());
  TEST_EQUAL(data->m_points[1].GetPoint(), MeterPoint(500, 0), ());
  TEST_EQUAL(data->m_points[3].GetPoint(), MeterPoint(600, 0), ());
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
  auto const joint = ResolveTrackEntry(MakeSourceTrack(source), MeterPoint(100, 0),
                                       mercator::DistanceOnEarth(MeterPoint(0, 0), MeterPoint(100, 0)));
  TEST(joint, ());
  auto const forward = BuildTrackFollowingData(source, *joint);
  TEST_EQUAL(forward->m_points.size(), 2, ());
  TEST_EQUAL(forward->m_points.front().GetAltitude(), 100, ());
  TEST_EQUAL(forward->m_points.back().GetAltitude(), 200, ());
  auto const reverse = BuildTrackFollowingData(source, *joint, true);
  TEST_EQUAL(reverse->m_points.size(), 3, ());
  TEST_EQUAL(reverse->m_points[0].GetAltitude(), 100, ());
  TEST_EQUAL(reverse->m_points[1].GetAltitude(), geometry::kInvalidAltitude, ());
  TEST_EQUAL(reverse->m_points[2].GetAltitude(), 50, ());
}

UNIT_TEST(TrackFollowData_ReverseHonorsLoopGapAndTrailheadDistance)
{
  double const degreesPerMeter = 1.0 / mercator::DistanceOnEarth({0, 0}, {1, 0});
  for (double const endpointGapM : {9.996, 10.004})
  {
    kml::TrackData source;
    source.m_geometry.AddLine(
        {{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 110}, {{endpointGapM * degreesPerMeter, 0}, 100}});
    source.m_geometry.AddTimestamps({});
    auto const track = MakeSourceTrack(source);
    // An entry 8 mm from the start has no usable geometry behind it on an open section.
    auto const entry = ResolveTrackEntryAt(track, 0.008);
    auto const finish = TrackSourcePosition{0, 1, 1.0};
    auto const expected = endpointGapM < 10.0 ? std::optional(finish) : std::nullopt;
    TEST(GetReverseTrackEntry(track, entry, finish) == expected, (endpointGapM));

    TrackSourcePosition const start{0, 0, 0.0};
    double const lengthM = track.GetLengthMeters();
    auto const nearFinish = ResolveTrackEntryAt(track, lengthM - 9.99);
    TEST(GetReverseTrackEntry(track, nearFinish, start) == (endpointGapM < 10.0 ? start : nearFinish), (endpointGapM));
    auto const interior = ResolveTrackEntryAt(track, lengthM - 10.01);
    TEST(GetReverseTrackEntry(track, interior, start) == interior, (endpointGapM));
  }
}

UNIT_TEST(TrackFollowData_ShortOpenSectionKeepsDistinctEndpoints)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(-1000, 0), 100}, {MeterPoint(-100, 0), 100}});
  source.m_geometry.AddTimestamps({});
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100}, {MeterPoint(5.4, 0), 100}});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);
  TEST(!GetReverseTrackEntry(track, {1, 0, 0.0}, {1, 0, 1.0}), ());
}

UNIT_TEST(TrackFollowData_ReverseKeepsTheMidpointOfAShortLoop)
{
  kml::TrackData source;
  source.m_geometry.AddLine({{MeterPoint(0, 0), 100},
                             {MeterPoint(4, 0), 100},
                             {MeterPoint(4, 4), 100},
                             {MeterPoint(0, 4), 100},
                             {MeterPoint(0, 0), 100}});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);
  auto const midway = ResolveTrackEntryAt(track, track.GetLengthMeters() / 2);
  TEST(GetReverseTrackEntry(track, midway, {0, 3, 1.0}) == midway, ());
}

UNIT_TEST(TrackFollowData_ReverseKeepsAnInteriorVisitToTheTrailhead)
{
  auto const trailhead = MeterPoint(0, 0);
  kml::TrackData source;
  source.m_geometry.AddLine(
      {{trailhead, 100}, {MeterPoint(100, 0), 110}, {trailhead, 100}, {MeterPoint(0, 100), 120}, {trailhead, 100}});
  source.m_geometry.AddTimestamps({});
  auto const track = MakeSourceTrack(source);
  auto const midway = ResolveTrackEntryAt(track, track.GetCumulativeLengths().front()[2]);
  TEST(GetReverseTrackEntry(track, midway, {0, 3, 1.0}) == midway, ());
}

UNIT_TEST(TrackFollowData_ReverseWaypointRouteKeepsCollocatedStops)
{
  kml::TrackData source;
  source.m_geometry.AddLine(
      {{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 110}, {MeterPoint(200, 0), 120}, {MeterPoint(300, 0), 130}});
  source.m_stops = {{MeterPoint(100, 0), "First"}, {MeterPoint(100, 0), "Second"}, {MeterPoint(200, 0), "Third"}};
  auto const data = BuildTrackFollowingData(source, {0, 2, 0.5}, true);
  TEST(data, ());
  TEST_EQUAL(data->m_points.size(), 4, ());
  TEST_EQUAL(data->m_points.front().GetAltitude(), 125, ());
  TEST_EQUAL(data->m_points.back().GetPoint(), MeterPoint(0, 0), ());
  TEST_EQUAL(data->m_stopPointIndexes, std::vector<size_t>({1, 2, 2}), ());
  TEST_EQUAL(data->m_points[1].GetPoint(), MeterPoint(200, 0), ());
  TEST_EQUAL(data->m_points[2].GetPoint(), MeterPoint(100, 0), ());
}
}  // namespace track_follow_data_tests
