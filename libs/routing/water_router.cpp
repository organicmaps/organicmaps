#include "routing/water_router.hpp"

#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>

namespace routing
{
namespace
{
RouterResultCode Walk(IRouter & pedestrian, m2::PointD const & from, m2::PointD const & to,
                      RouterDelegate const & delegate, ProgressCallback const & progress,
                      std::vector<m2::PointD> & points)
{
  RouterDelegate walkingDelegate(delegate.GetCancellable());
  walkingDelegate.SetProgressCallback(progress);
  walkingDelegate.SetPointCheckCallback([&](ms::LatLon const & point) { delegate.OnPointCheck(point); });
  RoutesResult walking;
  auto const code = pedestrian.CalculateRoute(Checkpoints(from, to), {}, false, false, walkingDelegate, walking);
  if (delegate.IsCancelled())
    return RouterResultCode::Cancelled;
  if (code != RouterResultCode::NoError)
    return code;
  if (!walking.IsValid())
    return RouterResultCode::RouteNotFound;
  Route const route(walking.GetActive());
  points = route.GetPoly().GetPoints();
  if (points.size() < 2 || mercator::DistanceOnEarth(points.front(), from) > 0.1 ||
      mercator::DistanceOnEarth(points.back(), to) > 0.1)
    return RouterResultCode::RouteNotFound;
  points.front() = from;
  points.back() = to;
  return RouterResultCode::NoError;
}
}  // namespace
RouterResultCode WaterRouter::CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                             RouterDelegate const & delegate, RoutesResult & result)
{
  auto const cancelled = [&delegate] { return delegate.IsCancelled(); };
  if (cancelled())
    return RouterResultCode::Cancelled;
  delegate.OnProgress(0);
  int lastProgress = 0;
  auto const report = [&](double value)
  {
    auto const percent = static_cast<int>(std::clamp(value, 0.0, 99.0));
    if (percent > lastProgress)
    {
      lastProgress = percent;
      delegate.OnProgress(percent);
    }
  };
  auto const geometry = m_geometryGetter(checkpoints, delegate);
  if (cancelled())
    return RouterResultCode::Cancelled;
  if (!geometry.m_complete)
    return RouterResultCode::RouteNotFound;

  report(30);
  Route route;
  std::vector<m2::PointD> points;
  std::vector<RouteSegment> segments;
  std::vector<Route::SubrouteAttrs> subroutes;
  RouteBase::WaterDistances distances;
  auto const junction = [](m2::PointD const & point)
  { return geometry::PointWithAltitude(point, geometry::kInvalidAltitude); };
  auto checkpointsPoints = checkpoints.GetPoints();
  std::vector<std::vector<m2::PointD>> approaches(checkpointsPoints.size());
  std::vector<std::vector<m2::PointD>> exits(checkpointsPoints.size());
  bool hasWalkingAccess = false;
  for (size_t i = 0; m_pedestrian && i < checkpointsPoints.size(); ++i)
  {
    water::Path membership;
    auto const point = checkpointsPoints[i];
    auto const membershipCode = water::FindPath(geometry, point, point, false, cancelled, membership);
    if (cancelled())
      return RouterResultCode::Cancelled;
    if (membershipCode == water::ResultCode::Success)
      continue;
    auto const entry = water::FindApproachPoint(geometry, point, cancelled);
    if (cancelled())
      return RouterResultCode::Cancelled;
    if (!entry)
      return i == 0                            ? RouterResultCode::StartPointNotFound
           : i + 1 == checkpointsPoints.size() ? RouterResultCode::EndPointNotFound
                                               : RouterResultCode::IntermediatePointNotFound;
    auto const progress = [&](float value) { report(30.0 + 10.0 * (i + value / 100.0) / checkpointsPoints.size()); };
    if (i > 0)
    {
      auto const code = Walk(*m_pedestrian, *entry, point, delegate, progress, exits[i]);
      if (code != RouterResultCode::NoError)
        return code;
    }
    if (i + 1 < checkpointsPoints.size())
    {
      auto const code = Walk(*m_pedestrian, point, *entry, delegate, progress, approaches[i]);
      if (code != RouterResultCode::NoError)
        return code;
    }
    checkpointsPoints[i] = *entry;
    hasWalkingAccess = true;
  }
  double const waterProgressStart = hasWalkingAccess ? 40.0 : 30.0;
  for (size_t i = 1; i < checkpointsPoints.size(); ++i)
  {
    auto const legProgress = [&](double value)
    { report(waterProgressStart + (98.0 - waterProgressStart) * (i - 1 + value) / (checkpointsPoints.size() - 1)); };
    water::Path path;
    auto const code = water::FindPath(geometry, checkpointsPoints[i - 1], checkpointsPoints[i],
                                      i == 1 && checkpoints.IsStartMyPosition(), cancelled, path,
                                      [&](double value) { legProgress(0.85 * value); });
    if (code == water::ResultCode::Cancelled)
      return RouterResultCode::Cancelled;
    if (code == water::ResultCode::OutsideWater)
    {
      water::Path membership;
      auto const startOutside = !(i == 1 && checkpoints.IsStartMyPosition()) &&
                                water::FindPath(geometry, checkpointsPoints[i - 1], checkpointsPoints[i - 1], false,
                                                cancelled, membership) == water::ResultCode::OutsideWater;
      if (cancelled())
        return RouterResultCode::Cancelled;
      if (startOutside)
        return i == 1 ? RouterResultCode::StartPointNotFound : RouterResultCode::IntermediatePointNotFound;
      return i + 1 == checkpointsPoints.size() ? RouterResultCode::EndPointNotFound
                                               : RouterResultCode::IntermediatePointNotFound;
    }
    if (code != water::ResultCode::Success)
      return RouterResultCode::RouteNotFound;
    auto const gaps = std::count(path.m_landGaps.begin(), path.m_landGaps.end(), true);
    if (gaps != 0)
    {
      if (!m_pedestrian)
        return RouterResultCode::RouteNotFound;
      auto const original = std::move(path);
      path = {};
      // The transfer was selected using a geometric lower bound. Its actual
      // walking cost does not certify the best combination of river endpoints.
      path.m_shortestConfirmed = false;
      path.m_points.push_back(original.m_points.front());
      auto const append = [&](m2::PointD const & point, bool land)
      {
        auto const & previous = path.m_points.back();
        if (previous == point)
          return;
        auto const distance = mercator::DistanceOnEarth(previous, point);
        (land ? path.m_landDistance : path.m_waterDistance) += distance;
        path.m_points.push_back(point);
        path.m_landGaps.push_back(land);
      };
      size_t processed = 0;
      for (size_t j = 1; j < original.m_points.size(); ++j)
      {
        if (!original.m_landGaps[j - 1])
        {
          append(original.m_points[j], false);
          continue;
        }
        std::vector<m2::PointD> walking;
        auto const code = Walk(*m_pedestrian, original.m_points[j - 1], original.m_points[j], delegate, [&](float value)
        { legProgress(0.85 + 0.15 * (processed + value / 100.0) / gaps); }, walking);
        if (code != RouterResultCode::NoError)
          return code;
        for (size_t k = 1; k < walking.size(); ++k)
          append(walking[k], true);
        ++processed;
      }
    }
    legProgress(1.0);
    auto & approach = approaches[i - 1];
    if (!approach.empty())
    {
      approach.back() = path.m_points.front();
      for (size_t j = 1; j < approach.size(); ++j)
        path.m_landDistance += mercator::DistanceOnEarth(approach[j - 1], approach[j]);
      path.m_landGaps.insert(path.m_landGaps.begin(), approach.size() - 1, true);
      approach.insert(approach.end(), path.m_points.begin() + 1, path.m_points.end());
      path.m_points = std::move(approach);
    }
    auto & exit = exits[i];
    if (!exit.empty())
    {
      exit.front() = path.m_points.back();
      for (size_t j = 1; j < exit.size(); ++j)
        path.m_landDistance += mercator::DistanceOnEarth(exit[j - 1], exit[j]);
      path.m_landGaps.insert(path.m_landGaps.end(), exit.size() - 1, true);
      path.m_points.insert(path.m_points.end(), exit.begin() + 1, exit.end());
    }
    if (points.empty())
      points.push_back(path.m_points.front());
    size_t const begin = segments.size();
    for (size_t j = 0; j < path.m_points.size(); ++j)
    {
      auto const & point = path.m_points[j];
      points.push_back(point);
      // Shared route presentation expects a terminal marker even for measurements.
      bool const destination = i + 1 == checkpointsPoints.size() && j + 1 == path.m_points.size();
      segments.emplace_back(Segment(kFakeNumMwmId, 0, 0, false),
                            turns::TurnItem(static_cast<uint32_t>(segments.size() + 1),
                                            destination ? turns::PedestrianDirection::ReachedYourDestination
                                                        : turns::PedestrianDirection::None),
                            junction(point), RouteSegment::RoadNameInfo{});
      segments.back().SetLandGap(j > 0 && path.m_landGaps[j - 1]);
    }
    subroutes.emplace_back(junction(path.m_points.front()), junction(path.m_points.back()), begin, segments.size());
    distances.m_shortestConfirmed = distances.m_shortestConfirmed && path.m_shortestConfirmed;
    distances.m_waterMeters += path.m_waterDistance;
    distances.m_landMeters += path.m_landDistance;
  }
  if (cancelled())
    return RouterResultCode::Cancelled;
  report(99);
  if (cancelled())
    return RouterResultCode::Cancelled;
  FillSegmentInfo(std::vector<double>(segments.size(), 0.0), segments);
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(std::move(subroutes), checkpoints.GetPassedIdx());
  route.SetGeometry(points.begin(), points.end());
  route.SetWaterDistances(distances);
  result.MakeFrom(GetName(), std::move(route));
  delegate.OnProgress(100);
  return RouterResultCode::NoError;
}
}  // namespace routing
