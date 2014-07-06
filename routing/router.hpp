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
  typedef function<void (Route const &)> ReadyCallback;

  virtual ~IRouter() {}

  virtual void SetFinalPoint(m2::PointD const & finalPt) = 0;
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback) = 0;
  virtual string GetName() const = 0;
};

}
