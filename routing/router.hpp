#pragma once

#include "geometry/point2d.hpp"

#include "std/function.hpp"
#include "std/string.hpp"


namespace routing
{

class Route;

class IRouter
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

  /// Callback takes ownership of passed route.
  typedef function<void (Route &, ResultCode)> ReadyCallback;

  virtual ~IRouter() {}

  virtual string GetName() const = 0;

  virtual void ClearState() {}
  virtual void ActivateAdditionalFeatures() {}

  virtual void CalculateRoute(m2::PointD const & startingPt, m2::PointD const & direction,
                              m2::PointD const & finalPt, ReadyCallback const & callback) = 0;
};

}
