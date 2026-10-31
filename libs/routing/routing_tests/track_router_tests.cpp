#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/router_delegate.hpp"
#include "routing/track_router.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace track_router_tests
{
using namespace routing;

using PWA = geometry::PointWithAltitude;

// Near the equator, MetersToMercator and DistanceOnEarth differ by about 2 m per nominal kilometer.
m2::PointD MeterPoint(double x, double y)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

// A straight track along the equator with |pointCount| points every |stepM| meters.
std::vector<PWA> MakeStraightTrack(size_t pointCount, double stepM)
{
  std::vector<PWA> track;
  track.reserve(pointCount);
  for (size_t i = 0; i < pointCount; ++i)
    track.emplace_back(MeterPoint(i * stepM, 0), geometry::kDefaultAltitudeMeters);
  return track;
}

std::shared_ptr<TrackFollowingData const> MakeData(std::vector<PWA> points, std::vector<size_t> stopPointIndexes = {})
{
  TrackFollowingData data;
  data.m_points = std::move(points);
  data.m_stopPointIndexes = std::move(stopPointIndexes);
  return std::make_shared<TrackFollowingData const>(std::move(data));
}

// Calculates the route and promotes it to a followed Route the same way RoutingSession::AssignRoute
// does (the promotion rebuilds the followed polyline from the subroutes and segments).
Route CalcRouteOnCheckpoints(TrackRouter & router, Checkpoints const & checkpoints)
{
  RouterDelegate delegate;
  RoutesResult result;
  TEST_EQUAL(router.CalculateRoute(checkpoints, m2::PointD::Zero(), false /* adjustToPrevRoute */,
                                   false /* needAlternatives */, delegate, result),
             RouterResultCode::NoError, ());
  TEST(result.IsValid(), ());
  return Route(result.GetActive());
}

Route CalcRoute(TrackRouter & router, std::vector<m2::PointD> && checkpointPoints)
{
  return CalcRouteOnCheckpoints(router, Checkpoints(std::move(checkpointPoints)));
}

std::vector<turns::TurnItem> GetTurns(Route const & route)
{
  std::vector<turns::TurnItem> turns;
  route.GetTurnsForTesting(turns);
  TEST_GREATER(turns.size(), 0, ());
  TEST(turns.back().IsTurnReachedYourDestination(), (turns.back().m_turn, turns.back().m_pedestrianTurn));
  return turns;
}

UNIT_TEST(TrackRouter_StraightTrackBasics)
{
  std::vector<PWA> const track = {{MeterPoint(0, 0), 100},
                                  {MeterPoint(100, 0), 110},
                                  {MeterPoint(200, 0), 120},
                                  {MeterPoint(300, 0), 130},
                                  {MeterPoint(400, 0), 140}};
  m2::PointD const start = MeterPoint(0, -30);  // 30 m off the track start.
  m2::PointD const finish = track.back().GetPoint();

  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {start, finish});
  TEST(route.IsValid(), ());

  // A geometric route does not invent a road name.
  TEST_EQUAL(route.GetRouteSegments().front().GetRoadNameInfo().m_name, "", ());
  TEST_EQUAL(route.GetRouteSegments().back().GetRoadNameInfo().m_name, "", ());

  // The lead-in from the off-track start + the whole track.
  auto const & poly = route.GetPoly();
  TEST_EQUAL(poly.GetSize(), track.size() + 1, ());
  TEST(poly.Front().EqualDxDy(start, mercator::kPointEqualityEps), ());
  TEST(poly.GetPoint(1).EqualDxDy(track.front().GetPoint(), mercator::kPointEqualityEps), ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 430.0, 2.0, ());
  TEST(route.GetTrackStartDistanceMeters().has_value(), ());
  TEST_ALMOST_EQUAL_ABS(*route.GetTrackStartDistanceMeters(), 30.0, 0.1, ());

  // One subroute for the single [start, finish] leg.
  TEST_EQUAL(route.GetSubrouteCount(), 1, ());
  TEST_EQUAL(route.GetSubrouteAttrs(0).GetBeginSegmentIdx(), 0, ());
  TEST_EQUAL(route.GetSubrouteAttrs(0).GetEndSegmentIdx(), route.GetRouteSegments().size(), ());

  // Track altitudes flow into the route (the lead-in start gets the projection's altitude).
  TEST(route.HaveAltitudes(), ());
  geometry::Altitudes altitudes;
  route.GetAltitudes(altitudes);
  geometry::Altitudes const expectedAltitudes = {100, 100, 110, 120, 130, 140};
  TEST_EQUAL(altitudes, expectedAltitudes, ());

  // Times are monotonic and scaled by the profile speed.
  auto const & segments = route.GetRouteSegments();
  double prevTimeS = 0.0;
  for (auto const & s : segments)
  {
    TEST_GREATER_OR_EQUAL(s.GetTimeFromBeginningSec(), prevTimeS, ());
    prevTimeS = s.GetTimeFromBeginningSec();
  }
  double const pedestrianTimeS = segments.back().GetTimeFromBeginningSec();
  TEST_ALMOST_EQUAL_ABS(pedestrianTimeS, 430.0 / (5.0 / 3.6), 5.0, ());

  // The straight track has no turns but ends with the pedestrian-profile destination item.
  auto const turns = GetTurns(route);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns.back().m_pedestrianTurn, turns::PedestrianDirection::ReachedYourDestination, ());
  TEST_EQUAL(turns.back().m_turn, turns::CarDirection::None, ());
  TEST_EQUAL(turns.back().m_index, static_cast<uint32_t>(poly.GetSize() - 1), ());

  // The car profile is 12 times faster (60 vs 5 km/h) and reports the destination via
  // CarDirection.
  TrackRouter carRouter(VehicleType::Car, MakeData(track));
  auto const carRoute = CalcRoute(carRouter, {start, finish});
  double const carTimeS = carRoute.GetRouteSegments().back().GetTimeFromBeginningSec();
  TEST_ALMOST_EQUAL_ABS(pedestrianTimeS / carTimeS, 12.0, 1.0E-6, ());
  TEST_EQUAL(GetTurns(carRoute).back().m_turn, turns::CarDirection::ReachedYourDestination, ());
}

UNIT_TEST(TrackRouter_OneSubroutePerCheckpointLeg)
{
  // A 1 km straight track with points every 100 m and stops at 300 m and 700 m.
  auto const track = MakeStraightTrack(11, 100.0);

  TrackRouter router(VehicleType::Pedestrian, MakeData(track, {3, 7} /* stopPointIndexes */));
  auto const route =
      CalcRoute(router, {track.front().GetPoint(), track[3].GetPoint(), track[7].GetPoint(), track.back().GetPoint()});

  // Exactly one subroute per checkpoint leg: RoutingSession::PassCheckpoints aligns
  // the subroute index with the checkpoint index.
  TEST_EQUAL(route.GetSubrouteCount(), 3, ());
  // The start coincides with the track start, so junction indexes match track point indexes.
  TEST_EQUAL(route.GetPoly().GetSize(), track.size(), ());
  TEST_EQUAL(route.GetTrackStartDistanceMeters(), std::optional<double>(0.0), ());

  size_t const expectedIdx[3][2] = {{0, 3}, {3, 7}, {7, 10}};
  for (size_t i = 0; i < 3; ++i)
  {
    auto const & attrs = route.GetSubrouteAttrs(i);
    TEST_EQUAL(attrs.GetBeginSegmentIdx(), expectedIdx[i][0], (i));
    TEST_EQUAL(attrs.GetEndSegmentIdx(), expectedIdx[i][1], (i));
  }
  TEST_EQUAL(route.GetSubrouteAttrs(2).GetEndSegmentIdx(), route.GetRouteSegments().size(), ());
  TEST(route.GetSubrouteAttrs(0).GetFinish().GetPoint().EqualDxDy(track[3].GetPoint(), mercator::kPointEqualityEps),
       ());
  TEST(route.GetSubrouteAttrs(1).GetStart().GetPoint().EqualDxDy(track[3].GetPoint(), mercator::kPointEqualityEps), ());
}

UNIT_TEST(TrackRouter_PassedCheckpointsRebuild)
{
  // A 1 km straight track with a stop at 500 m; rebuild after the stop was passed.
  auto const track = MakeStraightTrack(11, 100.0);

  TrackRouter router(VehicleType::Pedestrian, MakeData(track, {5} /* stopPointIndexes */));

  // Build the remaining route from the nearest projection at about 450 m.
  auto const route1 = CalcRoute(router, {MeterPoint(450, 10), track[5].GetPoint(), track.back().GetPoint()});
  TEST_EQUAL(route1.GetSubrouteCount(), 2, ());
  TEST_EQUAL(route1.GetCurrentSubrouteIdx(), 0, ());

  // Rebuild after the stop is passed: RoutingSession keeps all checkpoints and replaces
  // the passed one with the current position (see RoutingSession::RebuildRoute).
  Checkpoints checkpoints({MeterPoint(450, 10), track[5].GetPoint(), track.back().GetPoint()});
  checkpoints.PassNextPoint();
  checkpoints.SetPointFrom(MeterPoint(650, 10));

  auto const route2 = CalcRouteOnCheckpoints(router, checkpoints);
  TEST_EQUAL(route2.GetSubrouteCount(), 2, ());
  TEST_EQUAL(route2.GetCurrentSubrouteIdx(), 1, ());
  // The passed leg is an empty subroute, instantly reported as passed.
  TEST_EQUAL(route2.GetSubrouteAttrs(0).GetEndSegmentIdx(), 0, ());
  TEST(route2.IsSubroutePassed(0), ());
  TEST_EQUAL(route2.GetSubrouteAttrs(1).GetBeginSegmentIdx(), 0, ());
  TEST_EQUAL(route2.GetSubrouteAttrs(1).GetEndSegmentIdx(), route2.GetRouteSegments().size(), ());
  // The remaining way: a 10 m lead-in + 350 m of the track (segment distances are counted
  // from the current position, not from the passed subroutes' geometry).
  TEST_ALMOST_EQUAL_ABS(route2.GetRouteSegments().back().GetDistFromBeginningMeters(), 360.0, 2.0, ());
  // The promoted polyline starts at the current position after the passed checkpoints.
  TEST(route2.GetPoly().Front().EqualDxDy(MeterPoint(650, 10), mercator::kPointEqualityEps), ());
  TEST_ALMOST_EQUAL_ABS(route2.GetTotalDistanceMeters(), 360.0, 2.0, ());
}

UNIT_TEST(TrackRouter_StopBehindProjectionMakesEmptyLeg)
{
  // The position is beyond the (not yet passed) stop at 300 m: the stop's leg degenerates
  // to the lead-in and is passed as soon as the user is matched to the track.
  auto const track = MakeStraightTrack(11, 100.0);

  TrackRouter router(VehicleType::Pedestrian, MakeData(track, {3} /* stopPointIndexes */));
  auto const route = CalcRoute(router, {MeterPoint(650, 10), track[3].GetPoint(), track.back().GetPoint()});

  TEST_EQUAL(route.GetSubrouteCount(), 2, ());
  TEST_EQUAL(route.GetSubrouteAttrs(0).GetBeginSegmentIdx(), 0, ());
  TEST_EQUAL(route.GetSubrouteAttrs(0).GetEndSegmentIdx(), 1, ());
  TEST_EQUAL(route.GetSubrouteAttrs(1).GetBeginSegmentIdx(), 1, ());
  TEST_EQUAL(route.GetSubrouteAttrs(1).GetEndSegmentIdx(), route.GetRouteSegments().size(), ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 360.0, 2.0, ());
}

UNIT_TEST(TrackRouter_DuplicateStopsGetZeroLengthLeg)
{
  // Two stops on the same track point: the second leg still owns one (zero-length) segment,
  // so the subroute/checkpoint alignment holds.
  auto const track = MakeStraightTrack(11, 100.0);

  TrackRouter router(VehicleType::Pedestrian, MakeData(track, {5, 5} /* stopPointIndexes */));
  auto const route =
      CalcRoute(router, {track.front().GetPoint(), track[5].GetPoint(), track[5].GetPoint(), track.back().GetPoint()});

  TEST_EQUAL(route.GetSubrouteCount(), 3, ());
  TEST_EQUAL(route.GetSubrouteAttrs(1).GetSize(), 1, ());
  TEST(route.GetSubrouteAttrs(1).GetStart().GetPoint().EqualDxDy(route.GetSubrouteAttrs(1).GetFinish().GetPoint(),
                                                                 mercator::kPointEqualityEps),
       ());
  TEST_EQUAL(route.GetSubrouteAttrs(2).GetEndSegmentIdx(), route.GetRouteSegments().size(), ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 1000.0, 3.0, ());
}

UNIT_TEST(TrackRouter_DuplicateStopsOnCorner)
{
  for (auto const vehicle : {VehicleType::Car, VehicleType::Pedestrian})
  {
    for (double const y : {-200.0, 200.0})
    {
      // Coincident elevation samples and duplicate stops must still produce one corner hint.
      std::vector<PWA> const track = {
          {MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 10}, {MeterPoint(200, 0), 20}, {MeterPoint(200, y), 30}};
      TrackRouter router(vehicle, MakeData(track, {1, 1, 2, 2}));
      auto const route = CalcRoute(router, {track.front().GetPoint(), track[1].GetPoint(), track[1].GetPoint(),
                                            track[2].GetPoint(), track[2].GetPoint(), track.back().GetPoint()});
      auto const turns = GetTurns(route);
      TEST_EQUAL(turns.size(), 2, (vehicle, y));
      TEST_EQUAL(turns.front().m_index, 1U, (vehicle, y));
      if (vehicle == VehicleType::Pedestrian)
      {
        TEST_EQUAL(turns.front().m_pedestrianTurn,
                   y > 0 ? turns::PedestrianDirection::TurnLeft : turns::PedestrianDirection::TurnRight, (y));
      }
      else
      {
        TEST_EQUAL(turns.front().m_turn, y > 0 ? turns::CarDirection::TurnLeft : turns::CarDirection::TurnRight, (y));
      }
    }
  }
}

UNIT_TEST(TrackRouter_InvalidAltitudeIsNotInterpolated)
{
  // A track point without an elevation (e.g. a mixed GPX) carries geometry::kInvalidAltitude,
  // a large negative sentinel. Interpolating it would produce a plausible-looking junction
  // altitude and a route claiming a complete elevation profile that starts 16 km underground.
  std::vector<PWA> const track = {
      {MeterPoint(0, 0), geometry::kInvalidAltitude}, {MeterPoint(100, 0), 110}, {MeterPoint(200, 0), 120}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));

  // The projection falls in the middle of the segment whose start has no elevation; that track
  // point itself is not part of the route (the route starts at the projection).
  auto const route = CalcRoute(router, {MeterPoint(50, 10), track.back().GetPoint()});

  TEST(!route.HaveAltitudes(), ());
  geometry::Altitudes altitudes;
  route.GetAltitudes(altitudes);
  // The lead-in start and the projection both keep the sentinel; the known altitudes follow.
  geometry::Altitudes const expected = {geometry::kInvalidAltitude, geometry::kInvalidAltitude, 110, 120};
  TEST_EQUAL(altitudes, expected, ());
}

UNIT_TEST(TrackRouter_KnownVertexAfterUnknownPrefix)
{
  std::vector<PWA> const track = {
      {MeterPoint(0, 0), geometry::kInvalidAltitude}, {MeterPoint(100, 0), 100}, {MeterPoint(200, 0), 200}};
  for (auto const start : {MeterPoint(100, 0), MeterPoint(100, -10)})
  {
    TrackRouter router(VehicleType::Pedestrian, MakeData(track));
    auto const route = CalcRoute(router, {start, track.back().GetPoint()});
    TEST(route.HaveAltitudes(), (start));
    geometry::Altitudes altitudes;
    route.GetAltitudes(altitudes);
    TEST_EQUAL(altitudes.front(), 100, (start));
    TEST_EQUAL(altitudes.back(), 200, (start));
    TEST(std::ranges::none_of(altitudes, [](auto altitude) { return altitude == geometry::kInvalidAltitude; }),
         (altitudes));
  }
}

UNIT_TEST(TrackRouter_UnknownStartIsNotCompleteAltitude)
{
  std::vector<PWA> const track = {{MeterPoint(0, 0), geometry::kInvalidAltitude}, {MeterPoint(100, 0), 100}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
  TEST(!route.HaveAltitudes(), ());
  geometry::Altitudes altitudes;
  route.GetAltitudes(altitudes);
  TEST_EQUAL(altitudes, geometry::Altitudes({geometry::kInvalidAltitude, 100}), ());
}

UNIT_TEST(TrackRouter_KnownStartBeforeUnknownSegment)
{
  std::vector<PWA> const track = {
      {MeterPoint(0, 0), 100}, {MeterPoint(100, 0), geometry::kInvalidAltitude}, {MeterPoint(200, 0), 200}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
  TEST(!route.HaveAltitudes(), ());
  geometry::Altitudes altitudes;
  route.GetAltitudes(altitudes);
  TEST_EQUAL(altitudes, geometry::Altitudes({100, geometry::kInvalidAltitude, 200}), ());
}

UNIT_TEST(TrackRouter_CoincidentElevationSamples)
{
  std::vector<PWA> const track = {{MeterPoint(0, 0), 100},
                                  {MeterPoint(100, 0), 120},
                                  {MeterPoint(100, 0), geometry::kInvalidAltitude},
                                  {MeterPoint(100, 0), 150},
                                  {MeterPoint(200, 0), 200}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
  TEST(!route.HaveAltitudes(), ());
  geometry::Altitudes altitudes;
  route.GetAltitudes(altitudes);
  TEST_EQUAL(altitudes, geometry::Altitudes({100, 120, geometry::kInvalidAltitude, 150, 200}), ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 200.0, 1.0, ());
}

UNIT_TEST(TrackRouter_TurnGeneration)
{
  auto const getManeuvers = [](std::vector<PWA> const & track, VehicleType vehicleType)
  {
    TrackRouter router(vehicleType, MakeData(track));
    auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
    auto turns = GetTurns(route);
    turns.pop_back();  // Drop ReachedYourDestination, keep the actual maneuvers.
    return turns;
  };

  // A 90 degree corner: east, then north — a left turn at the corner vertex.
  std::vector<PWA> const leftCorner = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(200, 200), 0}};
  auto turns = getManeuvers(leftCorner, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::TurnLeft, ());
  TEST_EQUAL(turns[0].m_index, 1U, ());

  // The pedestrian profile reports the same maneuver via TurnItem::m_pedestrianTurn — that is
  // what the pedestrian navigation UI and TTS consume.
  turns = getManeuvers(leftCorner, VehicleType::Pedestrian);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::None, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());
  TEST_EQUAL(turns[0].m_index, 1U, ());

  // The mirrored corner is a right turn.
  std::vector<PWA> const rightCorner = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(200, -200), 0}};
  turns = getManeuvers(rightCorner, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::TurnRight, ());
  turns = getManeuvers(rightCorner, VehicleType::Pedestrian);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnRight, ());

  // A gentle 30 degree bend is not announced for any profile.
  std::vector<PWA> const gentleBend = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(373.2, 100), 0}};
  TEST_EQUAL(getManeuvers(gentleBend, VehicleType::Car).size(), 0, ());
  TEST_EQUAL(getManeuvers(gentleBend, VehicleType::Pedestrian).size(), 0, ());

  // 50 degrees is a slight turn; the pedestrian buckets have no slight/sharp granularity.
  std::vector<PWA> const slightBend = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(328.6, 153.2), 0}};
  turns = getManeuvers(slightBend, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::TurnSlightLeft, ());
  turns = getManeuvers(slightBend, VehicleType::Pedestrian);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());

  // 130 degrees is a sharp turn.
  std::vector<PWA> const sharpBend = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(71.4, 153.2), 0}};
  turns = getManeuvers(sharpBend, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::TurnSharpLeft, ());
  turns = getManeuvers(sharpBend, VehicleType::Pedestrian);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());

  // A straight out-and-back reversal vertex is a U-turn, not a sharp turn. The exactly
  // collinear case computes as +180 degrees and deterministically lands on the right side
  // (the announced phrase is the side-neutral "Make a U-turn").
  std::vector<PWA> const reversal = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(0, 0), 0}};
  turns = getManeuvers(reversal, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::UTurnRight, ());
  turns = getManeuvers(reversal, VehicleType::Pedestrian);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnRight, ());

  // A ~170 degree left hairpin keeps its side.
  std::vector<PWA> const leftHairpin = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(3.0, 34.7), 0}};
  turns = getManeuvers(leftHairpin, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::UTurnLeft, ());
}

UNIT_TEST(TrackRouter_CloseOppositeTurns)
{
  // Two real opposite corners must not be coalesced as detections of the same bend.
  std::vector<PWA> const track = {
      {MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(200, 30), 0}, {MeterPoint(400, 30), 0}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});

  auto const turns = GetTurns(route);
  TEST_EQUAL(turns.size(), 3, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());
  TEST_EQUAL(turns[0].m_index, 1U, ());
  TEST_EQUAL(turns[1].m_pedestrianTurn, turns::PedestrianDirection::TurnRight, ());
  TEST_EQUAL(turns[1].m_index, 2U, ());
}

UNIT_TEST(TrackRouter_BearingWindowInsideLastSegment)
{
  // The bearing window crosses the short middle edge and ends inside the final edge.
  // Using the final endpoint instead would make the first left bend look nearly straight.
  std::vector<PWA> const track = {
      {MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(200, 15), 0}, {MeterPoint(400, 15), 0}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
  auto const turns = GetTurns(route);
  TEST_EQUAL(turns.size(), 3, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());
  TEST_EQUAL(turns[1].m_pedestrianTurn, turns::PedestrianDirection::TurnRight, ());
}

UNIT_TEST(TrackRouter_FinishedCheckpoints)
{
  TrackRouter router(VehicleType::Pedestrian, MakeData(MakeStraightTrack(3, 100.0)));
  Checkpoints checkpoints(MeterPoint(0, 0), MeterPoint(200, 0));
  checkpoints.PassNextPoint();
  RouterDelegate delegate;
  RoutesResult result;
  TEST_EQUAL(router.CalculateRoute(checkpoints, m2::PointD::Zero(), false /* adjust */, false /* needAlternatives */,
                                   delegate, result),
             RouterResultCode::RouteNotFound, ());
}

UNIT_TEST(TrackRouter_CancelledBuild)
{
  auto const track = MakeStraightTrack(3, 100.0);
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  RouterDelegate delegate;
  delegate.Cancel();
  RoutesResult result;
  TEST_EQUAL(router.CalculateRoute(Checkpoints(track.front().GetPoint(), track.back().GetPoint()), m2::PointD::Zero(),
                                   false, false, delegate, result),
             RouterResultCode::Cancelled, ());
}

UNIT_TEST(TrackRouter_ReversedInput)
{
  auto track = MakeStraightTrack(3, 100.0);
  std::reverse(track.begin(), track.end());
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
  TEST(route.GetPoly().Front().EqualDxDy(track.front().GetPoint(), mercator::kPointEqualityEps), ());
  TEST(route.GetPoly().Back().EqualDxDy(track.back().GetPoint(), mercator::kPointEqualityEps), ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 200.0, 1.0, ());
}

UNIT_TEST(TrackRouter_HighLatitudePartialSegmentDistances)
{
  std::vector<PWA> const track = {{mercator::FromLatLon(60.0, 0.0), 100}, {mercator::FromLatLon(80.0, 0.0), 200}};
  auto const start = mercator::FromLatLon(75.0, 0.0);
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {start, track.back().GetPoint()});
  double const remaining = mercator::DistanceOnEarth(start, track.back().GetPoint());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), remaining, 1.0E-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetRouteSegments().back().GetDistFromBeginningMeters(), remaining, 1.0E-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalTimeSec(), remaining / (5.0 / 3.6), 1.0E-6, ());
}

UNIT_TEST(TrackRouter_AlreadyAtFinish)
{
  for (auto const & track : {MakeStraightTrack(3, 100.0), MakeStraightTrack(2, 0.0)})
  {
    TrackRouter router(VehicleType::Pedestrian, MakeData(track));
    auto const route = CalcRoute(router, {track.back().GetPoint(), track.back().GetPoint()});
    TEST(route.IsValid(), ());
    TEST_EQUAL(route.GetTotalDistanceMeters(), 0.0, ());
    TEST_EQUAL(route.GetRouteSegments().back().GetDistFromBeginningMeters(), 0.0, ());
    TEST_EQUAL(route.GetTotalTimeSec(), 0.0, ());
  }
}

}  // namespace track_router_tests
