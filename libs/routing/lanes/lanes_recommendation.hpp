#pragma once

#include "routing/lanes/lane_info.hpp"

#include <vector>

namespace routing
{
class RouteSegment;

namespace turns
{
enum class CarDirection;
}  // namespace turns
}  // namespace routing

namespace routing::turns::lanes
{
/// Selects lanes which are recommended for an end user.
void SelectRecommendedLanes(std::vector<RouteSegment> & routeSegments);

// Keep signatures in the header for testing purposes
namespace impl
{
bool SetRecommendedLaneWays(CarDirection carDirection, LanesInfo & lanesInfo);

bool SetRecommendedLaneWaysApproximately(CarDirection carDirection, LanesInfo & lanesInfo);

bool SetUnrestrictedLaneAsRecommended(CarDirection carDirection, LanesInfo & lanesInfo);
}  // namespace impl
}  // namespace routing::turns::lanes
