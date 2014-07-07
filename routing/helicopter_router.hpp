#pragma once

#include "router.hpp"

#include "../geometry/point2d.hpp"

namespace routing
{

class HelicopterRouter : public IRouter
{
  m2::PointD m_finalPt;

public:
  virtual string GetName() const { return "helicopter"; }

  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback);
};

} // routing
