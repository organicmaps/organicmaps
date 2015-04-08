#pragma once

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

#define ROUTING_CANCEL_INTERRUPT_CHECK if (IsCancelled()) return Cancelled;

namespace routing
{

class Route;

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
  /// @param startPt point to start routing
  /// @param direction start direction for routers with high cost of the turnarounds
  /// @param finalPt target point for route
  /// @param route result route
  /// @return ResultCode error code or NoError if route was initialised
  /// @see Cancellable
  virtual ResultCode CalculateRoute(m2::PointD const & startingPt, m2::PointD const & startDirection,
                                    m2::PointD const & finalPt, Route & route) = 0;
};

}
