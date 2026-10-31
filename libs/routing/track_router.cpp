#include "routing/track_router.hpp"

#include "routing/route.hpp"
#include "routing/turns.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace routing
{
namespace
{
// Bearing in/out window length around a track vertex for the geometric turn hints.
double constexpr kTurnWindowM = 20.0;
// No turn hint is generated when a window is clipped (by the track end or the lead-in)
// to less than this length: the bearing would be too noisy.
double constexpr kMinTurnWindowM = 5.0;
// Bearing delta thresholds in degrees for the turn hint buckets.
double constexpr kMinTurnAngleDeg = 45.0;
double constexpr kTurnAngleDeg = 60.0;
double constexpr kSharpTurnAngleDeg = 120.0;
// A turnaround (e.g. the reversal vertex of an out-and-back track) is announced as a U-turn.
double constexpr kUTurnAngleDeg = 150.0;
// Coalesce nearby detections of a bend in the same direction.
double constexpr kMinDistBetweenTurnsM = 50.0;

// Approximate profile speeds; the track carries no road travel-time data.
double GetSpeedKMpH(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Car: return 60.0;
  case VehicleType::Bicycle: return 15.0;
  // Transit has no geometric travel-time model; use walking speed.
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return 5.0;
  case VehicleType::Count: break;
  }
  CHECK(false, ("Invalid vehicle type", vehicleType));
  UNREACHABLE();
}

// Not expressed via turns::FindDirectionByAngle: its template definition lives in
// turns_generator_utils.cpp (linking would rely on an implicit instantiation of another TU),
// and its ">= lower bound" convention can't encode the inclusive kSharpTurnAngleDeg boundary.
turns::CarDirection GetTurnDirection(double angleDeg)
{
  double const absDeg = std::fabs(angleDeg);
  if (absDeg < kMinTurnAngleDeg)
    return turns::CarDirection::None;

  // Positive angle is a right turn (see turns::PiMinusTwoVectorsAngle); an exactly collinear
  // reversal computes as +180 and lands on UTurnRight — arbitrary, but the announced phrase
  // is the side-neutral "Make a U-turn".
  bool const right = angleDeg > 0.0;
  if (absDeg < kTurnAngleDeg)
    return right ? turns::CarDirection::TurnSlightRight : turns::CarDirection::TurnSlightLeft;
  if (absDeg <= kSharpTurnAngleDeg)
    return right ? turns::CarDirection::TurnRight : turns::CarDirection::TurnLeft;
  if (absDeg < kUTurnAngleDeg)
    return right ? turns::CarDirection::TurnSharpRight : turns::CarDirection::TurnSharpLeft;
  return right ? turns::CarDirection::UTurnRight : turns::CarDirection::UTurnLeft;
}

// Pedestrian navigation consumers (nav UI, TTS) read TurnItem::m_pedestrianTurn, which has no
// slight/sharp/U-turn granularity — all turn buckets collapse to plain left/right.
turns::PedestrianDirection ToPedestrianDirection(turns::CarDirection direction)
{
  switch (direction)
  {
  case turns::CarDirection::TurnSlightLeft:
  case turns::CarDirection::TurnLeft:
  case turns::CarDirection::TurnSharpLeft:
  case turns::CarDirection::UTurnLeft: return turns::PedestrianDirection::TurnLeft;
  case turns::CarDirection::TurnSlightRight:
  case turns::CarDirection::TurnRight:
  case turns::CarDirection::TurnSharpRight:
  case turns::CarDirection::UTurnRight: return turns::PedestrianDirection::TurnRight;
  case turns::CarDirection::ReachedYourDestination: return turns::PedestrianDirection::ReachedYourDestination;
  default: return turns::PedestrianDirection::None;
  }
}

// Point at |targetDistM| of the route distance within junctions [beginIdx, endIdx] (route
// distances are non-decreasing), interpolated on the boundary segment. Not expressed via
// m2::InterpolatePointAtDistance (which carries its own unify-TODO): it takes whole containers
// of plain points, while these PointWithAltitude junctions are searched on a subrange without copying.
m2::PointD PointAtDistance(std::vector<geometry::PointWithAltitude> const & junctions,
                           std::vector<double> const & routeDistM, size_t beginIdx, size_t endIdx, double targetDistM)
{
  auto const first = routeDistM.begin() + beginIdx;
  auto const last = routeDistM.begin() + endIdx + 1;
  size_t const j = std::lower_bound(first, last, targetDistM) - routeDistM.begin();
  if (j <= beginIdx)
    return junctions[beginIdx].GetPoint();
  if (j > endIdx)
    return junctions[endIdx].GetPoint();

  double const segLenM = routeDistM[j] - routeDistM[j - 1];
  double const f = segLenM < 1.0E-9 ? 0.0 : (targetDistM - routeDistM[j - 1]) / segLenM;
  auto const & from = junctions[j - 1].GetPoint();
  return from + (junctions[j].GetPoint() - from) * f;
}

// Find a bearing point on the track, without crossing the straight approach.
std::optional<m2::PointD> GetBearingPoint(std::vector<geometry::PointWithAltitude> const & junctions,
                                          std::vector<double> const & routeDistM, size_t trackStartIdx, size_t fromIdx,
                                          bool forward)
{
  size_t const limitIdx = forward ? junctions.size() - 1 : trackStartIdx;
  double const reachableM = std::fabs(routeDistM[limitIdx] - routeDistM[fromIdx]);
  if (reachableM >= kTurnWindowM)
  {
    double const targetM = forward ? routeDistM[fromIdx] + kTurnWindowM : routeDistM[fromIdx] - kTurnWindowM;
    return PointAtDistance(junctions, routeDistM, forward ? fromIdx : limitIdx, forward ? limitIdx : fromIdx, targetM);
  }

  if (reachableM < kMinTurnWindowM)
    return std::nullopt;
  return junctions[limitIdx].GetPoint();
}

// Geometric turn hints: the bearing delta over ~kTurnWindowM windows at each on-track vertex,
// bucketed into CarDirection (the pedestrian variant is derived on emission). Existing machinery
// (nav UI, TTS) handles the rest.
std::vector<turns::CarDirection> MakeTurnDirections(std::vector<geometry::PointWithAltitude> const & junctions,
                                                    std::vector<double> const & routeDistM, size_t projJunctionIdx,
                                                    RouterDelegate const & delegate)
{
  std::vector<turns::CarDirection> turnAtJunction(junctions.size(), turns::CarDirection::None);
  double lastTurnDistM = std::numeric_limits<double>::lowest();
  double lastTurnAngleDeg = 0.0;
  for (size_t k = projJunctionIdx + 1; k + 1 < junctions.size(); ++k)
  {
    if (delegate.IsCancelled())
      break;
    auto const inPoint = GetBearingPoint(junctions, routeDistM, projJunctionIdx, k, false /* forward */);
    if (!inPoint)
      continue;
    auto const outPoint = GetBearingPoint(junctions, routeDistM, projJunctionIdx, k, true /* forward */);
    if (!outPoint)
      continue;

    double const angleDeg = math::RadToDeg(turns::PiMinusTwoVectorsAngle(junctions[k].GetPoint(), *inPoint, *outPoint));
    auto const direction = GetTurnDirection(angleDeg);
    if (direction == turns::CarDirection::None)
      continue;
    if (routeDistM[k] - lastTurnDistM < kMinDistBetweenTurnsM && (angleDeg > 0.0) == (lastTurnAngleDeg > 0.0))
      continue;

    turnAtJunction[k] = direction;
    lastTurnDistM = routeDistM[k];
    lastTurnAngleDeg = angleDeg;
  }
  return turnAtJunction;
}

// PointWithAltitude::Interpolate blends geometry::kInvalidAltitude (-32768) like a real
// elevation, so a single track point without one would produce a plausible-looking junction
// altitude and a route claiming a complete elevation profile. Keep the sentinel instead: it is
// what RouteBase::SetRouteSegments looks for to report HaveAltitudes() == false.
geometry::PointWithAltitude InterpolateJunction(geometry::PointWithAltitude const & from,
                                                geometry::PointWithAltitude const & to, double f)
{
  if (f == 0.0)
    return from;
  if (f == 1.0)
    return to;
  auto result = from.Interpolate(to, f);
  if (from.GetAltitude() == geometry::kInvalidAltitude || to.GetAltitude() == geometry::kInvalidAltitude)
    result.SetAltitude(geometry::kInvalidAltitude);
  return result;
}
}  // namespace

TrackRouter::TrackRouter(VehicleType vehicleType, std::shared_ptr<TrackFollowingData const> data)
  : m_vehicleType(vehicleType)
  , m_speedMpS(measurement_utils::KmphToMps(GetSpeedKMpH(vehicleType)))
  , m_data(std::move(data))
{
  CHECK(m_data != nullptr, ());
  auto const & points = m_data->m_points;
  CHECK(std::is_sorted(m_data->m_stopPointIndexes.begin(), m_data->m_stopPointIndexes.end()), ());
  CHECK(m_data->m_stopPointIndexes.empty() || m_data->m_stopPointIndexes.back() < points.size(), ());
}

TrackRouter::Projection TrackRouter::ProjectPoint(m2::PointD const & point, RouterDelegate const & delegate) const
{
  auto const & points = m_data->m_points;
  ASSERT_GREATER_OR_EQUAL(points.size(), 2, ());

  Projection best;
  double bestDistSq = std::numeric_limits<double>::max();

  for (size_t i = 0; i + 1 < points.size(); ++i)
  {
    if (delegate.IsCancelled())
      return best;

    m2::PointD const & from = points[i].GetPoint();
    m2::PointD const & to = points[i + 1].GetPoint();

    // Project in Mercator space; retain source order when projections are equally close.
    m2::PointD const dir = to - from;
    double const lenSq = dir.SquaredLength();
    double const fraction = lenSq > 0.0 ? math::Clamp(m2::DotProduct(point - from, dir) / lenSq, 0.0, 1.0) : 0.0;

    m2::PointD const proj = from + dir * fraction;
    double const distSq = (point - proj).SquaredLength();
    if (distSq < bestDistSq)
    {
      bestDistSq = distSq;
      best.m_segIdx = i;
      best.m_fraction = fraction;
    }
  }

  // At least one segment exists, including when its endpoints coincide.
  ASSERT_LESS(bestDistSq, std::numeric_limits<double>::max(), ());
  return best;
}

TrackRouter::RouteGeometry TrackRouter::AssembleRouteGeometry(Checkpoints const & checkpoints, Projection const & proj,
                                                              RouterDelegate const & delegate) const
{
  auto const & trackPoints = m_data->m_points;
  size_t const numSubroutes = checkpoints.GetNumSubroutes();
  size_t const passedIdx = checkpoints.GetPassedIdx();

  m2::PointD const & startPoint = checkpoints.GetPointFrom();
  geometry::PointWithAltitude const projPoint =
      InterpolateJunction(trackPoints[proj.m_segIdx], trackPoints[proj.m_segIdx + 1], proj.m_fraction);
  RouteGeometry geom;
  auto & junctions = geom.m_junctions;
  size_t const maxJunctions = trackPoints.size() - proj.m_segIdx + (numSubroutes - passedIdx) + 1;
  junctions.reserve(maxJunctions);
  geom.m_distM.reserve(maxJunctions);

  junctions.emplace_back(startPoint, projPoint.GetAltitude());
  geom.m_distM.push_back(0.0);

  // Keep differing elevation samples even when their horizontal positions coincide.
  auto const appendJunction = [&](geometry::PointWithAltitude const & point)
  {
    if (!junctions.back().GetPoint().EqualDxDy(point.GetPoint(), mercator::kPointEqualityEps) ||
        junctions.back().GetAltitude() != point.GetAltitude())
    {
      double const distM =
          geom.m_distM.back() + mercator::DistanceOnEarth(junctions.back().GetPoint(), point.GetPoint());
      junctions.push_back(point);
      geom.m_distM.push_back(distM);
    }
  };

  appendJunction(projPoint);
  geom.m_projJunctionIdx = junctions.size() - 1;

  size_t trackCursor = proj.m_segIdx + 1;
  auto const appendTrackPointsUpTo = [&](size_t lastTrackIdx)
  {
    // |lastTrackIdx| is always a valid track point index (see the constructor asserts).
    for (; trackCursor <= lastTrackIdx; ++trackCursor)
    {
      if (delegate.IsCancelled())
        return;
      appendJunction(trackPoints[trackCursor]);
    }
  };

  geom.m_legEndJunctions.reserve(numSubroutes - passedIdx);
  size_t prevEnd = 0;
  for (size_t leg = passedIdx; leg < numSubroutes; ++leg)
  {
    if (delegate.IsCancelled())
      return geom;
    // Every non-final leg ends at a stop: CalculateRoute() asserts one checkpoint per stop.
    appendTrackPointsUpTo(leg + 1 == numSubroutes ? trackPoints.size() - 1 : m_data->m_stopPointIndexes[leg]);

    size_t end = junctions.size() - 1;
    if (end == prevEnd)
    {
      // Give the degenerate leg a zero-length segment of its own.
      junctions.push_back(junctions.back());
      geom.m_distM.push_back(geom.m_distM.back());
      end = junctions.size() - 1;
    }
    geom.m_legEndJunctions.push_back(end);
    prevEnd = end;
  }

  return geom;
}

RouterResultCode TrackRouter::CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & /* startDirection */,
                                             bool /* adjustToPrevRoute */, bool /* needAlternatives */,
                                             RouterDelegate const & delegate, RoutesResult & result)
{
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;
  if (m_data->m_points.size() < 2 || checkpoints.IsFinished())
    return RouterResultCode::RouteNotFound;

  CHECK_EQUAL(checkpoints.GetPoints().size(), m_data->m_stopPointIndexes.size() + 2, (checkpoints));

  Projection const proj = ProjectPoint(checkpoints.GetPointFrom(), delegate);
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;

  // Build [start, projection point, rest of the track],
  // split into one leg per remaining checkpoint at the stops' points. A degenerate leg (a stop
  // behind the projection or several stops on one track point) gets a zero-length segment.
  RouteGeometry const geom = AssembleRouteGeometry(checkpoints, proj, delegate);
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;

  auto const & junctions = geom.m_junctions;
  auto const & routeDistM = geom.m_distM;
  size_t const junctionsCount = junctions.size();
  size_t const segmentsCount = junctionsCount - 1;

  // Geometric turn hints.
  auto const turnAtJunction = MakeTurnDirections(junctions, routeDistM, geom.m_projJunctionIdx, delegate);
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;

  // Fake segment IDs keep geometric guidance separate from road-graph matching.
  Segment const fakeSegment(kFakeNumMwmId, 0, 0, false /* forward */);
  RouteSegment::RoadNameInfo trackRoadInfo;
  bool const pedestrian = m_vehicleType == VehicleType::Pedestrian;

  std::vector<RouteSegment> routeSegments;
  routeSegments.reserve(segmentsCount);
  double distMerc = 0.0;
  for (size_t s = 0; s < segmentsCount; ++s)
  {
    if (delegate.IsCancelled())
      return RouterResultCode::Cancelled;
    size_t const junctionIdx = s + 1;
    auto const direction =
        junctionIdx == junctionsCount - 1 ? turns::CarDirection::ReachedYourDestination : turnAtJunction[junctionIdx];
    auto const index = static_cast<uint32_t>(junctionIdx);
    turns::TurnItem const turn =
        pedestrian ? turns::TurnItem(index, ToPedestrianDirection(direction)) : turns::TurnItem(index, direction);
    routeSegments.emplace_back(fakeSegment, turn, junctions[junctionIdx], trackRoadInfo);
    // Segment distances and times are counted from the route start (junction 0), so the lead-in
    // is included. FillSegmentInfo is not used: it accumulates only between segment junctions
    // and would silently drop the lead-in (junction 0 owns no RouteSegment by convention).
    distMerc += junctions[junctionIdx].GetPoint().Length(junctions[junctionIdx - 1].GetPoint());
    routeSegments[s].SetDistancesAndTime(routeDistM[junctionIdx], distMerc, routeDistM[junctionIdx] / m_speedMpS);
  }

  Route route;
  route.SetTrackStartDistanceMeters(geom.m_distM[geom.m_projJunctionIdx]);
  route.SetRouteSegments(std::move(routeSegments));

  // Exactly one subroute per checkpoint leg: RoutingSession::PassCheckpoints aligns
  // subroute indexes with checkpoint indexes. Passed legs become empty subroutes.
  std::vector<Route::SubrouteAttrs> subroutes;
  subroutes.reserve(checkpoints.GetNumSubroutes());
  // Deviation from IndexRouter::PushPassedSubroutes: passed legs reference the CURRENT start
  // junction (with its altitude), not the original checkpoints. Promoting a RouteBase to a
  // followed Route (see RoutingSession::AssignRoute) rebuilds the polyline from
  // subroutes.front().GetStart() + segment junctions, so stale checkpoints would prepend a
  // phantom edge; nothing else consumes the passed subroutes' endpoints.
  for (size_t i = 0; i < checkpoints.GetPassedIdx(); ++i)
    subroutes.emplace_back(junctions.front(), junctions.front(), 0 /* beginSegmentIdx */, 0 /* endSegmentIdx */);
  size_t begin = 0;
  for (size_t const end : geom.m_legEndJunctions)
  {
    subroutes.emplace_back(junctions[begin], junctions[end], begin, end);
    begin = end;
  }

  route.SetSubroutes(std::move(subroutes), checkpoints.GetPassedIdx());

  // No SetGeometry(): RoutesResult::MakeFrom() keeps only the RouteBase slice, so the
  // FollowedPolyline built here would be thrown away — RoutingSession::AssignRoute rebuilds it
  // from the segments (Route(RouteBase const &)). Skipping it saves a haversine per track point
  // and the whole polyline copy on every build and rebuild.
  result.MakeFrom(GetName(), std::move(route));
  return RouterResultCode::NoError;
}
}  // namespace routing
