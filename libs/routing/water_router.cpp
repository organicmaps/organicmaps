#include "routing/water_router.hpp"

#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"

#include <algorithm>

namespace routing
{
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
  auto const & checkpointsPoints = checkpoints.GetPoints();
  for (size_t i = 1; i < checkpointsPoints.size(); ++i)
  {
    water::Path path;
    auto const code = water::FindPath(geometry, checkpointsPoints[i - 1], checkpointsPoints[i],
                                      i == 1 && checkpoints.IsStartMyPosition(), cancelled, path, [&](double value)
    { report(30.0 + 68.0 * (i - 1 + value) / (checkpointsPoints.size() - 1)); });
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
