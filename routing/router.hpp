#pragma once

#include "router_delegate.hpp"

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

namespace routing
{

using TCountryFileFn = function<string(m2::PointD const &)>;

class Route;

/// Routing engine type.
enum class RouterType
{
  Vehicle = 0, /// For OSRM vehicle routing
  Pedestrian   /// For A star pedestrian routing
};

string ToString(RouterType type);

class IRouter
{
public:
  /// Routing possible statuses enumeration.
  /// \warning  this enum has JNI mirror!
  /// \see android/src/com/mapswithme/maps/data/RoutingResultCodesProcessor.java
  enum ResultCode // TODO(mgsergio) enum class
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
    FileTooOld = 11
  };

  virtual ~IRouter() {}

  /// Return unique name of a router implementation.
  virtual string GetName() const = 0;

  /// Clear all temporary buffers.
  virtual void ClearState() {}

  /// Override this function with routing implementation.
  /// It will be called in separate thread and only one function will processed in same time.
  /// @warning please support Cancellable interface calls. You must stop processing when it is true.
  ///
  /// @param startPoint point to start routing
  /// @param startDirection start direction for routers with high cost of the turnarounds
  /// @param finalPoint target point for route
  /// @param delegate callback functions and cancellation flag
  /// @param route result route
  /// @return ResultCode error code or NoError if route was initialised
  /// @see Cancellable
  virtual ResultCode CalculateRoute(m2::PointD const & startPoint,
                                    m2::PointD const & startDirection,
                                    m2::PointD const & finalPoint, RouterDelegate const & delegate,
                                    Route & route) = 0;
};

}  // namespace routing
