#include "routing/routing_quality/waypoints.hpp"
#include "routing/routing_quality/utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <utility>

using namespace std;

namespace routing_quality
{
namespace metrics
{
Similarity CompareByNumberOfMatchedWaypoints(routing::FollowedPolyline && polyline, ReferenceRoutes && candidates)
{
  auto constexpr kMaxDistanceFromRouteM = 15;
  Similarity bestResult = 0.0;
  for (size_t j = 0; j < candidates.size(); ++j)
  {
    routing::FollowedPolyline current = polyline;
    auto const & candidate = candidates[j];
    auto const & waypoints = candidate.m_waypoints;
    auto const size = waypoints.size();
    CHECK_GREATER(size, 0, ());
    size_t numberOfErrors = 0;
    for (size_t i = 0; i < size; ++i)
    {
      auto const & ll = waypoints[i];
      m2::RectD const rect = MercatorBounds::MetresToXY(ll.lon, ll.lat, kMaxDistanceFromRouteM /* metresR */);
      auto const iter = current.UpdateProjection(rect);
      if (iter.IsValid())
      {
        auto const distFromRouteM = MercatorBounds::DistanceOnEarth(iter.m_pt, MercatorBounds::FromLatLon(ll));
        if (distFromRouteM <= kMaxDistanceFromRouteM)
          continue;
      }

      LOG(LINFO, ("Can't find point", ll, "with index", i));
      ++numberOfErrors;
    }

    CHECK_LESS_OR_EQUAL(numberOfErrors, size, ());
    auto const result = ((size - numberOfErrors) / static_cast<double>(size)) * candidate.m_factor;
    LOG(LINFO, ("Matching result", result, "for route with index", j));
    bestResult = max(bestResult, result);
  }

  LOG(LINFO, ("Best result", bestResult));
  return bestResult;
}
}  // namespace metrics

Similarity CheckWaypoints(RouteParams && params, ReferenceRoutes && candidates)
{
  return metrics::CompareByNumberOfMatchedWaypoints(GetRouteFollowedPolyline(move(params)), move(candidates));
}
}  // namespace routing_quality
