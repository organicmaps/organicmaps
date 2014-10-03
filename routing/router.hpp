#pragma once

#include "../geometry/point2d.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"


namespace routing
{

class Route;

class IRouter
{
public:
  enum ResultCode
  {
    NoError = 0,
    InconsistentMWMandRoute,
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

  virtual void SetFinalPoint(m2::PointD const & finalPt) = 0;
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback) = 0;
};

}
