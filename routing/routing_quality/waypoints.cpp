#include "routing/routing_quality/waypoints.hpp"

#include "routing/base/followed_polyline.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <utility>

namespace routing_quality
{
namespace metrics
{
Similarity CompareByNumberOfMatchedWaypoints(routing::FollowedPolyline polyline, Waypoints const & waypoints)
{
  double constexpr kMaxDistanceFromRouteM = 15.0;

  auto const size = waypoints.size();
  CHECK_GREATER(size, 0, ());
  size_t numberOfErrors = 0;
  for (size_t i = 0; i < size; ++i)
  {
    auto const & ll = waypoints[i];
    m2::RectD const rect = mercator::MetersToXY(ll.m_lon, ll.m_lat, kMaxDistanceFromRouteM /* metresR */);
    auto const iter = polyline.UpdateProjection(rect);
    if (iter.IsValid())
    {
      auto const distFromRouteM = mercator::DistanceOnEarth(iter.m_pt, mercator::FromLatLon(ll));
      if (distFromRouteM <= kMaxDistanceFromRouteM)
        continue;
    }

    ++numberOfErrors;
  }

  CHECK_LESS_OR_EQUAL(numberOfErrors, size, ());
  auto const result = ((size - numberOfErrors) / static_cast<double>(size));

  return result;
}

Similarity CompareByNumberOfMatchedWaypoints(routing::FollowedPolyline const & polyline, ReferenceRoutes && candidates)
{
  Similarity bestResult = 0.0;
  for (size_t j = 0; j < candidates.size(); ++j)
  {
    auto const result = CompareByNumberOfMatchedWaypoints(polyline, candidates[j]);
    bestResult = std::max(bestResult, result);
  }

  LOG(LDEBUG, ("Best result:", bestResult));
  return bestResult;
}
}  // namespace metrics

Similarity CheckWaypoints(Params const & params, ReferenceRoutes && referenceRoutes)
{
  auto & builder = routing::routes_builder::RoutesBuilder::GetSimpleRoutesBuilder();
  auto result = builder.ProcessTask(params);

  return metrics::CompareByNumberOfMatchedWaypoints(result.GetRoutes().back().m_followedPolyline,
                                                    std::move(referenceRoutes));
}

bool CheckRoute(Params const & params, ReferenceRoutes && referenceRoutes)
{
  return CheckWaypoints(params, std::move(referenceRoutes)) == 1.0;
}

bool CheckCarRoute(ms::LatLon const & start, ms::LatLon const & finish, ReferenceRoutes && referenceTracks)
{
  Params const params(routing::VehicleType::Car, start, finish);
  return CheckRoute(params, std::move(referenceTracks));
}
}  // namespace routing_quality
