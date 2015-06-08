#pragma once

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

namespace routing
{

class Route;

/// Routing engine type.
enum RouterType
{
  Vehicle,    /// For OSRM vehicle routing
  Pedestrian  /// For A star pedestrian routing
};

class IRouter : public my::Cancellable
{
public:
  enum ResultCode
  {
    NoError = 0,
    Cancelled,
    NoCurrentPosition,
    InconsistentMWMandRoute,
    RouteFileNotExist,
    StartPointNotFound,
    EndPointNotFound,
    PointsInDifferentMWM,
    RouteNotFound,
    InternalError
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
  /// @param route result route
  /// @return ResultCode error code or NoError if route was initialised
  /// @see Cancellable
  virtual ResultCode CalculateRoute(m2::PointD const & startPoint,
                                    m2::PointD const & startDirection,
                                    m2::PointD const & finalPoint, Route & route) = 0;
};

typedef function<void(m2::PointD const &)> TRoutingVisualizerFn;

}  // namespace routing
