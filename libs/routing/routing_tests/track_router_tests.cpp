#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/router_delegate.hpp"
#include "routing/track_router.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/math.hpp"

#include <cmath>
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

std::vector<PWA> SampleFlatTrack(std::vector<m2::PointD> const & vertices, size_t subdivisions)
{
  std::vector<PWA> track;
  for (size_t i = 0; i + 1 < vertices.size(); ++i)
    for (size_t j = 0; j < subdivisions; ++j)
      track.emplace_back(vertices[i] + (vertices[i + 1] - vertices[i]) * (static_cast<double>(j) / subdivisions));
  track.emplace_back(vertices.back());
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
  TEST(!router.IsTrafficSensitive(), ());

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

  // The approach starts at the selected entry's altitude.
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

  // Car and bicycle profiles use their own speeds and report the destination via CarDirection.
  for (auto const & [vehicle, speedKMpH] : {std::pair{VehicleType::Car, 60.0}, {VehicleType::Bicycle, 15.0}})
  {
    TrackRouter profileRouter(vehicle, MakeData(track));
    auto const profileRoute = CalcRoute(profileRouter, {start, finish});
    double const profileTimeS = profileRoute.GetRouteSegments().back().GetTimeFromBeginningSec();
    TEST_ALMOST_EQUAL_ABS(pedestrianTimeS / profileTimeS, speedKMpH / 5.0, 1.0E-6, (vehicle));
    TEST_EQUAL(GetTurns(profileRoute).back().m_turn, turns::CarDirection::ReachedYourDestination, (vehicle));
  }
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
  // Junction 1 ends the zero-length approach at the track start.
  TEST_EQUAL(route.GetPoly().GetSize(), track.size() + 1, ());
  TEST_EQUAL(route.GetTrackStartDistanceMeters(), std::optional<double>(0.0), ());

  size_t const expectedIdx[3][2] = {{0, 4}, {4, 8}, {8, 11}};
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

UNIT_TEST(TrackRouter_StopAtEntryKeepsCheckpointLeg)
{
  auto const track = MakeStraightTrack(11, 100.0);
  TrackRouter router(VehicleType::Pedestrian, MakeData(track, {0}));
  for (double const approachM : {0.0, 10.0})
  {
    auto const route =
        CalcRoute(router, {MeterPoint(0, -approachM), track.front().GetPoint(), track.back().GetPoint()});
    TEST_EQUAL(route.GetSubrouteCount(), 2, (approachM));
    TEST_EQUAL(route.GetSubrouteAttrs(0).GetBeginSegmentIdx(), 0, (approachM));
    TEST_EQUAL(route.GetSubrouteAttrs(0).GetEndSegmentIdx(), 1, (approachM));
    TEST_EQUAL(route.GetSubrouteAttrs(1).GetBeginSegmentIdx(), 1, (approachM));
    TEST_EQUAL(route.GetSubrouteAttrs(1).GetEndSegmentIdx(), route.GetRouteSegments().size(), (approachM));
    TEST_ALMOST_EQUAL_ABS(*route.GetTrackStartDistanceMeters(), approachM, 0.1, (approachM));
    TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 1000.0 + approachM, 3.0, (approachM));
  }
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
      TEST_EQUAL(turns.front().m_index, 2U, (vehicle, y));
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

UNIT_TEST(TrackRouter_Altitudes)
{
  struct TestCase
  {
    char const * m_name;
    std::vector<PWA> m_track;
    m2::PointD m_start;
    bool m_haveAltitudes;
    geometry::Altitudes m_altitudes;
    double m_distanceM;
  };
  auto constexpr kUnknown = geometry::kInvalidAltitude;
  std::vector<PWA> const unknownEntry = {
      {MeterPoint(50, 0), kUnknown}, {MeterPoint(100, 0), 100}, {MeterPoint(200, 0), 200}};
  std::vector<PWA> const knownEntry = {{MeterPoint(100, 0), 100}, {MeterPoint(200, 0), 200}};
  std::vector<PWA> const unknownStart = {{MeterPoint(0, 0), kUnknown}, {MeterPoint(100, 0), 100}};
  std::vector<PWA> const unknownSegment = {
      {MeterPoint(0, 0), 100}, {MeterPoint(100, 0), kUnknown}, {MeterPoint(200, 0), 200}};
  std::vector<PWA> const coincident = {{MeterPoint(0, 0), 100},
                                       {MeterPoint(100, 0), 120},
                                       {MeterPoint(100, 0), kUnknown},
                                       {MeterPoint(100, 0), 150},
                                       {MeterPoint(200, 0), 200}};
  // Prepared samples retain their altitude availability through the approach.
  TestCase const cases[] = {
      {"UnknownPreparedEntry", unknownEntry, MeterPoint(50, 10), false, {kUnknown, kUnknown, 100, 200}, 160.0},
      {"KnownPreparedEntry", knownEntry, MeterPoint(100, 0), true, {100, 100, 200}, 100.0},
      {"KnownPreparedEntryWithApproach", knownEntry, MeterPoint(100, -10), true, {100, 100, 200}, 110.0},
      {"UnknownStartIsNotCompleteAltitude", unknownStart, MeterPoint(0, 0), false, {kUnknown, kUnknown, 100}, 100.0},
      {"KnownStartBeforeUnknownSegment", unknownSegment, MeterPoint(0, 0), false, {100, 100, kUnknown, 200}, 200.0},
      {"CoincidentElevationSamples", coincident, MeterPoint(0, 0), false, {100, 100, 120, kUnknown, 150, 200}, 200.0}};
  for (auto const & test : cases)
  {
    TrackRouter router(VehicleType::Pedestrian, MakeData(test.m_track));
    auto const route = CalcRoute(router, {test.m_start, test.m_track.back().GetPoint()});
    TEST_EQUAL(route.HaveAltitudes(), test.m_haveAltitudes, (test.m_name));
    geometry::Altitudes altitudes;
    route.GetAltitudes(altitudes);
    TEST_EQUAL(altitudes, test.m_altitudes, (test.m_name));
    TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), test.m_distanceM, 1.0, (test.m_name));
  }
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
  TEST_EQUAL(turns[0].m_index, 2U, ());

  // The pedestrian profile reports the same maneuver via TurnItem::m_pedestrianTurn — that is
  // what the pedestrian navigation UI and TTS consume.
  turns = getManeuvers(leftCorner, VehicleType::Pedestrian);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::None, ());
  TEST_EQUAL(turns[0].m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, ());
  TEST_EQUAL(turns[0].m_index, 2U, ());

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

  // 55 and 130 degrees use the same normal-turn bucket as road routes.
  std::vector<PWA> const normalBend = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(314.7, 163.8), 0}};
  turns = getManeuvers(normalBend, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::TurnLeft, ());

  std::vector<PWA> const sharpBend = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(71.4, 153.2), 0}};
  turns = getManeuvers(sharpBend, VehicleType::Car);
  TEST_EQUAL(turns.size(), 1, ());
  TEST_EQUAL(turns[0].m_turn, turns::CarDirection::TurnLeft, ());
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
  TEST_EQUAL(turns[0].m_index, 2U, ());
  TEST_EQUAL(turns[1].m_pedestrianTurn, turns::PedestrianDirection::TurnRight, ());
  TEST_EQUAL(turns[1].m_index, 3U, ());
}

UNIT_TEST(TrackRouter_NoisyTurnaroundProducesOneHint)
{
  // A 100 m out-and-back with 2 m sample spacing and 10 cm lateral noise.
  std::vector<PWA> track;
  for (size_t i = 0; i <= 100; ++i)
    track.emplace_back(MeterPoint(2.0 * (i <= 50 ? i : 100 - i), i % 2 == 0 ? 0.1 : -0.1));

  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    TrackRouter router(vehicle, MakeData(track));
    auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
    auto const turns = GetTurns(route);
    TEST_EQUAL(turns.size(), 2, (vehicle));
    if (vehicle == VehicleType::Pedestrian)
    {
      TEST(turns.front().m_pedestrianTurn == turns::PedestrianDirection::TurnLeft ||
               turns.front().m_pedestrianTurn == turns::PedestrianDirection::TurnRight,
           (vehicle));
    }
    else
    {
      TEST(turns.front().m_turn == turns::CarDirection::UTurnLeft ||
               turns.front().m_turn == turns::CarDirection::UTurnRight,
           (vehicle));
    }
  }
}

UNIT_TEST(TrackRouter_CloseOppositeUTurns)
{
  for (double const side : {-1.0, 1.0})
  {
    // Distinct 170-degree switchbacks only 15 m apart have opposing incoming bearings.
    double const angle = math::DegToRad(side * 170.0);
    auto const firstTurn = MeterPoint(100, 0);
    auto const secondTurn = firstTurn + MeterPoint(15 * cos(angle), 15 * sin(angle));
    auto const track = SampleFlatTrack({MeterPoint(0, 0), firstTurn, secondTurn, secondTurn + MeterPoint(100, 0)}, 100);
    for (auto const vehicle : {VehicleType::Car, VehicleType::Pedestrian})
    {
      TrackRouter router(vehicle, MakeData(track));
      auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
      auto const turns = GetTurns(route);
      TEST_EQUAL(turns.size(), 3, (vehicle, side));
      if (vehicle == VehicleType::Pedestrian)
      {
        TEST_EQUAL(turns[0].m_pedestrianTurn,
                   side > 0 ? turns::PedestrianDirection::TurnLeft : turns::PedestrianDirection::TurnRight, (side));
        TEST_EQUAL(turns[1].m_pedestrianTurn,
                   side > 0 ? turns::PedestrianDirection::TurnRight : turns::PedestrianDirection::TurnLeft, (side));
      }
      else
      {
        TEST_EQUAL(turns[0].m_turn, side > 0 ? turns::CarDirection::UTurnLeft : turns::CarDirection::UTurnRight,
                   (side));
        TEST_EQUAL(turns[1].m_turn, side > 0 ? turns::CarDirection::UTurnRight : turns::CarDirection::UTurnLeft,
                   (side));
      }
    }
  }
}

UNIT_TEST(TrackRouter_CloseSameDirectionTurns)
{
  for (auto const vehicle : {VehicleType::Car, VehicleType::Pedestrian})
  {
    for (double const y : {-30.0, 30.0})
    {
      std::vector<m2::PointD> const vertices = {MeterPoint(0, 0), MeterPoint(200, 0), MeterPoint(200, y),
                                                MeterPoint(0, y)};
      for (size_t const subdivisions : {1, 200})
      {
        auto const track = SampleFlatTrack(vertices, subdivisions);
        TrackRouter router(vehicle, MakeData(track));
        auto const route = CalcRoute(router, {vertices.front(), vertices.back()});
        auto const turns = GetTurns(route);
        TEST_EQUAL(turns.size(), 3, (vehicle, y, subdivisions));
        for (size_t i = 0; i < 2; ++i)
        {
          TEST(route.GetPoly().GetPoint(turns[i].m_index).EqualDxDy(vertices[i + 1], mercator::kPointEqualityEps),
               (vehicle, y, subdivisions, i));
          if (vehicle == VehicleType::Pedestrian)
          {
            TEST_EQUAL(turns[i].m_pedestrianTurn,
                       y > 0 ? turns::PedestrianDirection::TurnLeft : turns::PedestrianDirection::TurnRight,
                       (y, subdivisions, i));
          }
          else
          {
            TEST_EQUAL(turns[i].m_turn, y > 0 ? turns::CarDirection::TurnLeft : turns::CarDirection::TurnRight,
                       (y, subdivisions, i));
          }
        }
      }
    }
  }
}

UNIT_TEST(TrackRouter_TurnHintsIgnoreCollinearSamples)
{
  struct TestCase
  {
    m2::PointD m_finish;
    turns::CarDirection m_left;
    turns::CarDirection m_right;
  };
  TestCase const cases[] = {
      {MeterPoint(200, 200), turns::CarDirection::TurnLeft, turns::CarDirection::TurnRight},
      {MeterPoint(314.7, 163.8), turns::CarDirection::TurnLeft, turns::CarDirection::TurnRight},
      {MeterPoint(71.4, 153.2), turns::CarDirection::TurnLeft, turns::CarDirection::TurnRight},
      {MeterPoint(32.3, 108.9), turns::CarDirection::TurnSharpLeft, turns::CarDirection::TurnSharpRight},
      {MeterPoint(3, 34.7), turns::CarDirection::UTurnLeft, turns::CarDirection::UTurnRight}};
  for (auto const & test : cases)
  {
    for (double const side : {-1.0, 1.0})
    {
      std::vector<m2::PointD> const vertices = {
          MeterPoint(0, 0), MeterPoint(200, 0), {test.m_finish.x, side * test.m_finish.y}};
      for (size_t const subdivisions : {1, 200})
      {
        auto const track = SampleFlatTrack(vertices, subdivisions);
        TrackRouter router(VehicleType::Car, MakeData(track));
        auto const route = CalcRoute(router, {vertices.front(), vertices.back()});
        auto const turns = GetTurns(route);
        TEST_EQUAL(turns.size(), 2, (test.m_finish, side, subdivisions));
        TEST_EQUAL(turns.front().m_turn, side > 0 ? test.m_left : test.m_right, (test.m_finish, side, subdivisions));
        TEST(route.GetPoly().GetPoint(turns.front().m_index).EqualDxDy(vertices[1], mercator::kPointEqualityEps),
             (test.m_finish, side, subdivisions));
      }
    }
  }
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

UNIT_TEST(TrackRouter_StandstillNoiseDoesNotGenerateTurns)
{
  for (double const noiseM : {2.0, 3.0})
  {
    auto track = MakeStraightTrack(201, 1.0);
    // Sixty stationary fixes accumulate distance while remaining within a few meters.
    for (size_t i = 0; i < 15; ++i)
    {
      track.emplace_back(MeterPoint(200 + noiseM, 0));
      track.emplace_back(MeterPoint(200, noiseM));
      track.emplace_back(MeterPoint(200 - noiseM, 0));
      track.emplace_back(MeterPoint(200, -noiseM));
    }
    for (size_t i = 200; i <= 400; ++i)
      track.emplace_back(MeterPoint(i, 0));

    for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
    {
      TrackRouter router(vehicle, MakeData(track));
      auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
      TEST_EQUAL(GetTurns(route).size(), 1, (vehicle, noiseM));
    }
  }
}

UNIT_TEST(TrackRouter_ClippedBearingWindows)
{
  for (double const legM : {4.0, 8.0, 20.0})
  {
    auto const track = SampleFlatTrack({MeterPoint(0, 0), MeterPoint(legM, 0), MeterPoint(legM, legM)}, 1);
    TrackRouter router(VehicleType::Pedestrian, MakeData(track));
    auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
    auto const turns = GetTurns(route);
    TEST_EQUAL(turns.size(), legM < 5.0 ? 1 : 2, (legM));
    if (legM >= 5.0)
      TEST_EQUAL(turns.front().m_pedestrianTurn, turns::PedestrianDirection::TurnLeft, (legM));
  }
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

UNIT_TEST(TrackRouter_SelectedEntryAndRemainderArePreserved)
{
  struct TestCase
  {
    char const * m_name;
    std::vector<PWA> m_track;
    m2::PointD m_start;
    double m_distanceM;
  };
  std::vector<PWA> const loop = {{MeterPoint(0, 0), 0},
                                 {MeterPoint(200, 0), 0},
                                 {MeterPoint(200, 200), 0},
                                 {MeterPoint(0, 200), 0},
                                 {MeterPoint(0, 0), 0}};
  std::vector<PWA> const outAndBack = {{MeterPoint(0, 0), 0}, {MeterPoint(200, 0), 0}, {MeterPoint(0, 0), 0}};
  TestCase const cases[] = {{"ClosedLoopAtEntry", loop, MeterPoint(0, 0), 800.0},
                            {"ClosedLoopFromLaterVertex", loop, MeterPoint(200, 0), 1000.0},
                            {"OutAndBackFromOverlap", outAndBack, MeterPoint(100, 0), 500.0},
                            {"StartAtFinish", MakeStraightTrack(3, 100.0), MeterPoint(200, 0), 400.0}};
  for (auto const & test : cases)
  {
    TrackRouter router(VehicleType::Pedestrian, MakeData(test.m_track));
    auto const route = CalcRoute(router, {test.m_start, test.m_track.back().GetPoint()});
    double const approachM = mercator::DistanceOnEarth(test.m_start, test.m_track.front().GetPoint());
    TEST_EQUAL(route.GetPoly().GetSize(), test.m_track.size() + 1, (test.m_name));
    TEST(route.GetPoly().Front().EqualDxDy(test.m_start, mercator::kPointEqualityEps), (test.m_name));
    TEST(route.GetPoly().GetPoint(1).EqualDxDy(test.m_track.front().GetPoint(), mercator::kPointEqualityEps),
         (test.m_name));
    TEST(route.GetPoly().Back().EqualDxDy(test.m_track.back().GetPoint(), mercator::kPointEqualityEps), (test.m_name));
    TEST_ALMOST_EQUAL_ABS(*route.GetTrackStartDistanceMeters(), approachM, 1e-6, (test.m_name));
    TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), test.m_distanceM, 2.0, (test.m_name));
  }
}

UNIT_TEST(TrackRouter_GpsNearFinishKeepsPreparedEntry)
{
  std::vector<PWA> const track = {{MeterPoint(0, 0), 100}, {MeterPoint(100, 0), 110}, {MeterPoint(200, 0), 120}};
  auto const start = MeterPoint(190, 0);
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {start, track.back().GetPoint()});
  TEST_EQUAL(route.GetPoly().GetSize(), track.size() + 1, ());
  TEST_EQUAL(route.GetPoly().GetPoint(1), track.front().GetPoint(), ());
  TEST_ALMOST_EQUAL_ABS(*route.GetTrackStartDistanceMeters(), 190.0, 1.0, ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), 390.0, 1.0, ());
  geometry::Altitudes altitudes;
  route.GetAltitudes(altitudes);
  TEST_EQUAL(altitudes, geometry::Altitudes({100, 100, 110, 120}), ());
}

UNIT_TEST(TrackRouter_DistantApproachKeepsCompleteRemainder)
{
  auto const track = MakeStraightTrack(3, 100.0);
  auto const start = mercator::FromLatLon(60, 60);
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {start, track.back().GetPoint()});
  double const approachM = mercator::DistanceOnEarth(start, track.front().GetPoint());
  double const remainderM = mercator::DistanceOnEarth(track.front().GetPoint(), track.back().GetPoint());
  TEST_EQUAL(route.GetPoly().GetSize(), track.size() + 1, ());
  TEST_EQUAL(route.GetPoly().GetPoint(1), track.front().GetPoint(), ());
  TEST_ALMOST_EQUAL_ABS(*route.GetTrackStartDistanceMeters(), approachM, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), approachM + remainderM, 1e-6, ());
}

UNIT_TEST(TrackRouter_HighLatitudePreparedRemainderDistances)
{
  auto const start = mercator::FromLatLon(75.0, 0.0);
  std::vector<PWA> const track = {{start, 100}, {mercator::FromLatLon(80.0, 0.0), 200}};
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {start, track.back().GetPoint()});
  double const remaining = mercator::DistanceOnEarth(start, track.back().GetPoint());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalDistanceMeters(), remaining, 1.0E-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetRouteSegments().back().GetDistFromBeginningMeters(), remaining, 1.0E-6, ());
  TEST_ALMOST_EQUAL_ABS(route.GetTotalTimeSec(), remaining / (5.0 / 3.6), 1.0E-6, ());
}

UNIT_TEST(TrackRouter_ZeroLengthRemainder)
{
  auto const track = MakeStraightTrack(2, 0.0);
  TrackRouter router(VehicleType::Pedestrian, MakeData(track));
  auto const route = CalcRoute(router, {track.front().GetPoint(), track.back().GetPoint()});
  TEST(route.IsValid(), ());
  TEST_EQUAL(route.GetTotalDistanceMeters(), 0.0, ());
  TEST_EQUAL(route.GetRouteSegments().back().GetDistFromBeginningMeters(), 0.0, ());
  TEST_EQUAL(route.GetTotalTimeSec(), 0.0, ());
}

}  // namespace track_router_tests
