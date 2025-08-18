#pragma once

#include "routing/turns.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace routing
{
class Route;

/// Routing possible statuses enumeration.
/// \warning  this enum has JNI mirror!
/// \see android/src/app/organicmaps/maps/routing/ResultCodesHelper.java
// TODO(bykoianko): Items become obsolete now should be removed from the enum.
enum class RouterResultCode
{
  NoError = 0,
  Cancelled = 1,
  NoCurrentPosition = 2,
  InconsistentMWMandRoute = 3,
  RouteFileNotExist = 4,
  StartPointNotFound = 5,
  EndPointNotFound = 6,
  PointsInDifferentMWM = 7,
  RouteNotFound = 8,
  NeedMoreMaps = 9,
  InternalError = 10,
  FileTooOld = 11,
  IntermediatePointNotFound = 12,
  TransitRouteNotFoundNoNetwork = 13,
  TransitRouteNotFoundTooLongPedestrian = 14,
  RouteNotFoundRedressRouteError = 15,
  HasWarnings = 16,
};

enum class SessionState
{
  NoValidRoute,      // No valid route: no route after application launching or the route was removed.
  RouteBuilding,     // We requested a route and wait when it will be built. User may be following
                     // the previous route.
  RouteNotStarted,   // Route is built but the user isn't on it.
  OnRoute,           // User follows the route.
  RouteNeedRebuild,  // User left the route.
  RouteFinished,     // Destination point is reached but the session isn't closed.
  RouteNoFollowing,  // Route is built but following mode has been disabled.
  RouteRebuilding,   // We requested a route rebuild and wait when it will be rebuilt.
                     // User may following the previous route.
};

/*
 * NoValidRoute -> RouteBuilding         // start route building
 * RouteBuilding -> RouteNotStarted      // route is built in case of building a new route
 * RouteRebuilding -> RouteNotStarted    // route is built in case of rebuilding
 * RouteNotStarted -> OnRoute            // user started following the route
 * RouteNotStarted -> RouteNeedRebuild   // user doesn't like the route.
 * OnRoute -> RouteNeedRebuild           // user moves away from route - need to rebuild
 * RouteNeedRebuild -> RouteRebuilding   // the route is in process of rebuilding or
 *                                       // while rebuilding an error happens
 * RouteRebuilding -> OnRoute            // following along route after rebuilding
 * OnRoute -> RouteNoFollowing           // following mode was disabled. Router doesn't track position
 * OnRoute -> RouteFinished              // user reached the end of route
 * OnRoute -> RouteBuilding              // while moving along a route user makes a new route
 */

using CheckpointCallback = std::function<void(size_t passedCheckpointIdx)>;
using NeedMoreMapsCallback = std::function<void(uint64_t, std::set<std::string> const &)>;
using PointCheckCallback = std::function<void(ms::LatLon const &)>;
using ProgressCallback = std::function<void(float)>;
using ReadyCallback = std::function<void(Route const &, RouterResultCode)>;
using ReadyCallbackOwnership = std::function<void(std::shared_ptr<Route>, RouterResultCode)>;
using RemoveRouteCallback = std::function<void(RouterResultCode)>;
using RouteCallback = std::function<void(Route const &)>;
using ChangeSessionStateCallback = std::function<void(SessionState previous, SessionState current)>;
using SpeedCameraShowCallback = std::function<void(m2::PointD const & point, double cameraSpeedKmPH)>;
using SpeedCameraClearCallback = std::function<void()>;
using OnNewTurn = std::function<void()>;

inline std::string ToString(RouterResultCode code)
{
  switch (code)
  {
  case RouterResultCode::NoError: return "NoError";
  case RouterResultCode::Cancelled: return "Cancelled";
  case RouterResultCode::NoCurrentPosition: return "NoCurrentPosition";
  case RouterResultCode::InconsistentMWMandRoute: return "InconsistentMWMandRoute";
  case RouterResultCode::RouteFileNotExist: return "RouteFileNotExist";
  case RouterResultCode::StartPointNotFound: return "StartPointNotFound";
  case RouterResultCode::EndPointNotFound: return "EndPointNotFound";
  case RouterResultCode::PointsInDifferentMWM: return "PointsInDifferentMWM";
  case RouterResultCode::RouteNotFound: return "RouteNotFound";
  case RouterResultCode::InternalError: return "InternalError";
  case RouterResultCode::NeedMoreMaps: return "NeedMoreMaps";
  case RouterResultCode::FileTooOld: return "FileTooOld";
  case RouterResultCode::IntermediatePointNotFound: return "IntermediatePointNotFound";
  case RouterResultCode::TransitRouteNotFoundNoNetwork: return "TransitRouteNotFoundNoNetwork";
  case RouterResultCode::TransitRouteNotFoundTooLongPedestrian: return "TransitRouteNotFoundTooLongPedestrian";
  case RouterResultCode::RouteNotFoundRedressRouteError: return "RouteNotFoundRedressRouteError";
  case RouterResultCode::HasWarnings: return "HasWarnings";
  }

  std::string result = "Unknown RouterResultCode: " + std::to_string(static_cast<int>(code));
  ASSERT(false, (result));
  return result;
}

inline std::string DebugPrint(RouterResultCode code)
{
  return ToString(code);
}

// This define should be set to see the spread of A* waves on the map.
// #define SHOW_ROUTE_DEBUG_MARKS
}  // namespace routing
