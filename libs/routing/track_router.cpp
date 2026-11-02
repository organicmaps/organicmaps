#include "routing/track_router.hpp"

#include "routing/route.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator_utils.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace routing
{
namespace
{
// Bound cancellation latency without locking for every track point.
size_t constexpr kCancellationCheckInterval = 1024;
// Junction 0 is the current start; junction 1 is the caller-selected track entry.
size_t constexpr kTrackStartIdx = 1;
// Local bearing windows give basic hints; broad bends can be understated or missed.
double constexpr kTurnWindowM = 20.0;
// No turn hint is generated when a window is clipped (by the track end or the lead-in)
// to less than this length: the bearing would be too noisy.
double constexpr kMinTurnWindowM = 5.0;
// With no road candidates, only substantial bearing changes merit a hint.
double constexpr kMinTurnAngleDeg = 45.0;
// A turnaround (e.g. the reversal vertex of an out-and-back track) is announced as a U-turn.
double constexpr kUTurnAngleDeg = 150.0;

// Approximate profile speeds; the track carries no road travel-time data.
double GetSpeedKMpH(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Car: return 60.0;
  case VehicleType::Bicycle: return 15.0;
  case VehicleType::Pedestrian: return 5.0;
  case VehicleType::Transit:
  case VehicleType::Count: break;
  }
  CHECK(false, ("Invalid vehicle type", vehicleType));
  UNREACHABLE();
}

turns::CarDirection GetTurnDirection(double angleDeg)
{
  double const absDeg = fabs(angleDeg);
  if (absDeg < kMinTurnAngleDeg)
    return turns::CarDirection::None;

  // Positive angles turn right; a collinear reversal uses +180 degrees.
  if (absDeg >= kUTurnAngleDeg)
    return angleDeg > 0.0 ? turns::CarDirection::UTurnRight : turns::CarDirection::UTurnLeft;
  return turns::IntermediateDirection(angleDeg);
}

// Interpolate within junctions [beginIdx, endIdx] using non-decreasing route distances.
m2::PointD PointAtDistance(RouteJunctions const & junctions, std::vector<double> const & routeDistM, size_t beginIdx,
                           size_t endIdx, double targetDistM)
{
  auto const first = routeDistM.begin() + beginIdx;
  auto const last = routeDistM.begin() + endIdx + 1;
  size_t const j = std::lower_bound(first, last, targetDistM) - routeDistM.begin();
  ASSERT_LESS_OR_EQUAL(j, endIdx, (targetDistM));
  if (j == beginIdx)
    return junctions[beginIdx].GetPoint();

  double const segLenM = routeDistM[j] - routeDistM[j - 1];
  ASSERT_GREATER(segLenM, 0.0, ());
  double const f = (targetDistM - routeDistM[j - 1]) / segLenM;
  auto const & from = junctions[j - 1].GetPoint();
  return from + (junctions[j].GetPoint() - from) * f;
}

// Find a bearing point on the track, without crossing the straight approach.
std::optional<m2::PointD> GetBearingPoint(RouteJunctions const & junctions, std::vector<double> const & routeDistM,
                                          size_t fromIdx, bool forward)
{
  size_t const limitIdx = forward ? junctions.size() - 1 : kTrackStartIdx;
  double const reachableM = fabs(routeDistM[limitIdx] - routeDistM[fromIdx]);
  if (reachableM >= kTurnWindowM)
  {
    double const targetM = forward ? routeDistM[fromIdx] + kTurnWindowM : routeDistM[fromIdx] - kTurnWindowM;
    return PointAtDistance(junctions, routeDistM, forward ? fromIdx : limitIdx, forward ? limitIdx : fromIdx, targetM);
  }

  if (reachableM < kMinTurnWindowM)
    return std::nullopt;
  return junctions[limitIdx].GetPoint();
}

// Detect bends from bearing windows around each on-track vertex.
std::vector<double> MakeTurnAngles(RouteJunctions const & junctions, std::vector<double> const & routeDistM,
                                   RouterDelegate const & delegate)
{
  std::vector<double> turnAtJunction(junctions.size(), 0.0);
  std::optional<size_t> lastTurnIdx;
  double lastTurnAngleDeg = 0.0;
  m2::PointD lastInDirection;
  for (size_t k = kTrackStartIdx + 1; k + 1 < junctions.size(); ++k)
  {
    if (k % kCancellationCheckInterval == 0 && delegate.IsCancelled())
      break;
    auto const inPoint = GetBearingPoint(junctions, routeDistM, k, false /* forward */);
    if (!inPoint)
      continue;
    auto const outPoint = GetBearingPoint(junctions, routeDistM, k, true /* forward */);
    if (!outPoint)
      continue;

    // Stationary GPS noise accumulates track length without a meaningful bearing.
    auto const & vertex = junctions[k].GetPoint();
    double const inWindowM = std::min(kTurnWindowM, routeDistM[k] - routeDistM[kTrackStartIdx]);
    double const outWindowM = std::min(kTurnWindowM, routeDistM.back() - routeDistM[k]);
    if (mercator::DistanceOnEarth(*inPoint, vertex) < inWindowM / 2 ||
        mercator::DistanceOnEarth(vertex, *outPoint) < outWindowM / 2)
      continue;

    double const angleDeg = math::RadToDeg(turns::PiMinusTwoVectorsAngle(vertex, *inPoint, *outPoint));
    if (fabs(angleDeg) < kMinTurnAngleDeg)
      continue;
    // Nearby bearing windows can detect the same bend. Keep its strongest hint;
    // a corner beyond one window can produce its own hint.
    // A single U-turn can cross +/-180 degrees with noise. Its incoming bearings agree,
    // whereas consecutive opposite switchbacks have opposing incoming bearings.
    if (lastTurnIdx && routeDistM[k] - routeDistM[*lastTurnIdx] < kTurnWindowM &&
        ((angleDeg > 0.0) == (lastTurnAngleDeg > 0.0) ||
         (fabs(angleDeg) >= kUTurnAngleDeg && fabs(lastTurnAngleDeg) >= kUTurnAngleDeg &&
          m2::DotProduct(vertex - *inPoint, lastInDirection) > 0.0)))
    {
      if (fabs(angleDeg) <= fabs(lastTurnAngleDeg))
        continue;
      turnAtJunction[*lastTurnIdx] = 0.0;
    }

    turnAtJunction[k] = angleDeg;
    lastTurnIdx = k;
    lastTurnAngleDeg = angleDeg;
    lastInDirection = vertex - *inPoint;
  }
  return turnAtJunction;
}

struct RouteGeometry
{
  RouteJunctions m_junctions;
  std::vector<double> m_distM;
  std::vector<size_t> m_legEndJunctions;
};

RouteGeometry AssembleRouteGeometry(TrackFollowingData const & data, Checkpoints const & checkpoints,
                                    RouterDelegate const & delegate)
{
  auto const & trackPoints = data.m_points;
  size_t const numSubroutes = checkpoints.GetNumSubroutes();

  m2::PointD const & startPoint = checkpoints.GetPointFrom();
  auto const & entryPoint = trackPoints.front();
  RouteGeometry geom;
  auto & junctions = geom.m_junctions;
  size_t const maxJunctions = trackPoints.size() + numSubroutes + 1;
  junctions.reserve(maxJunctions);
  geom.m_distM.reserve(maxJunctions);

  junctions.emplace_back(startPoint, entryPoint.GetAltitude());
  geom.m_distM.push_back(0.0);
  // Even an empty approach makes the entry altitude a segment endpoint.
  junctions.push_back(entryPoint);
  geom.m_distM.push_back(mercator::DistanceOnEarth(startPoint, entryPoint.GetPoint()));

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

  size_t trackCursor = 1;
  auto const appendTrackPointsUpTo = [&](size_t lastTrackIdx)
  {
    for (; trackCursor <= lastTrackIdx; ++trackCursor)
    {
      if (trackCursor % kCancellationCheckInterval == 0 && delegate.IsCancelled())
        return;
      appendJunction(trackPoints[trackCursor]);
    }
  };

  geom.m_legEndJunctions.reserve(numSubroutes);
  size_t prevEnd = 0;
  for (size_t leg = 0; leg < numSubroutes; ++leg)
  {
    if (delegate.IsCancelled())
      return geom;
    // Each non-final checkpoint corresponds to an ordered stop index.
    appendTrackPointsUpTo(leg + 1 == numSubroutes ? trackPoints.size() - 1 : data.m_stopPointIndexes[leg]);

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
}  // namespace

TrackRouter::TrackRouter(VehicleType vehicleType, std::shared_ptr<TrackFollowingData const> data)
  : m_data(std::move(data))
  , m_vehicleType(vehicleType)
{
  CHECK(
      vehicleType == VehicleType::Car || vehicleType == VehicleType::Bicycle || vehicleType == VehicleType::Pedestrian,
      (vehicleType));
  CHECK(m_data != nullptr, ());
  auto const & points = m_data->m_points;
  CHECK_GREATER_OR_EQUAL(points.size(), 2, ());
  CHECK(std::is_sorted(m_data->m_stopPointIndexes.begin(), m_data->m_stopPointIndexes.end()), ());
  CHECK(m_data->m_stopPointIndexes.empty() || m_data->m_stopPointIndexes.back() < points.size(), ());
}

RouterResultCode TrackRouter::CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & /* startDirection */,
                                             bool /* adjustToPrevRoute */, bool /* needAlternatives */,
                                             RouterDelegate const & delegate, RoutesResult & result)
{
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;

  CHECK_EQUAL(checkpoints.GetPassedIdx(), 0, (checkpoints));
  CHECK_EQUAL(checkpoints.GetPoints().size(), m_data->m_stopPointIndexes.size() + 2, (checkpoints));

#if defined(DEBUG)
  for (size_t leg = 0; leg < checkpoints.GetNumSubroutes(); ++leg)
  {
    size_t const pointIdx =
        leg + 1 == checkpoints.GetNumSubroutes() ? m_data->m_points.size() - 1 : m_data->m_stopPointIndexes[leg];
    auto const & checkpoint = checkpoints.GetPoint(leg + 1);
    auto const & trackPoint = m_data->m_points[pointIdx].GetPoint();
    ASSERT(checkpoint.EqualDxDy(trackPoint, mercator::kPointEqualityEps), (leg, checkpoint, trackPoint));
  }
#endif

  RouteGeometry const geom = AssembleRouteGeometry(*m_data, checkpoints, delegate);
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;

  auto const & junctions = geom.m_junctions;
  auto const & routeDistM = geom.m_distM;
  size_t const junctionsCount = junctions.size();
  size_t const segmentsCount = junctionsCount - 1;

  auto const turnAtJunction = MakeTurnAngles(junctions, routeDistM, delegate);
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;

  // The track has no road features; fake segments also keep position and bearing from snapping onto it.
  Segment const fakeSegment(kFakeNumMwmId, 0, 0, false /* forward */);
  RouteSegment::RoadNameInfo trackRoadInfo;

  std::vector<RouteSegment> routeSegments;
  routeSegments.reserve(segmentsCount);
  bool const pedestrian = m_vehicleType == VehicleType::Pedestrian;
  double const speedMpS = measurement_utils::KmphToMps(GetSpeedKMpH(m_vehicleType));
  double distMerc = 0.0;
  for (size_t s = 0; s < segmentsCount; ++s)
  {
    if (s % kCancellationCheckInterval == 0 && delegate.IsCancelled())
      return RouterResultCode::Cancelled;
    size_t const junctionIdx = s + 1;
    bool const atFinish = junctionIdx == junctionsCount - 1;
    double const angle = turnAtJunction[junctionIdx];
    auto const index = static_cast<uint32_t>(junctionIdx);
    turns::TurnItem turn(index, turns::CarDirection::None);
    if (pedestrian)
    {
      if (atFinish)
        turn.m_pedestrianTurn = turns::PedestrianDirection::ReachedYourDestination;
      else if (angle != 0.0)
        turn.m_pedestrianTurn = turns::IntermediateDirectionPedestrian(angle);
    }
    else
      turn.m_turn = atFinish ? turns::CarDirection::ReachedYourDestination : GetTurnDirection(angle);
    routeSegments.emplace_back(fakeSegment, turn, junctions[junctionIdx], trackRoadInfo);
    // Include the approach from junction 0 in cumulative segment distances and times.
    distMerc += junctions[junctionIdx].GetPoint().Length(junctions[junctionIdx - 1].GetPoint());
    routeSegments[s].SetDistancesAndTime(routeDistM[junctionIdx], distMerc, routeDistM[junctionIdx] / speedMpS);
  }

  Route route;
  route.SetTrackStartDistanceMeters(routeDistM[kTrackStartIdx]);
  route.SetRouteSegments(std::move(routeSegments));

  std::vector<Route::SubrouteAttrs> subroutes;
  subroutes.reserve(checkpoints.GetNumSubroutes());
  size_t begin = 0;
  for (size_t const end : geom.m_legEndJunctions)
  {
    subroutes.emplace_back(junctions[begin], junctions[end], begin, end);
    begin = end;
  }

  route.SetSubroutes(std::move(subroutes));

  // Promotion to a followed Route reconstructs its polyline from the subroutes and segments.
  result.MakeFrom(GetName(), std::move(route));
  return RouterResultCode::NoError;
}
}  // namespace routing
