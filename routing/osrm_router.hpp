#pragma once

#include "router.hpp"

namespace routing
{

class OsrmRouter : public IRouter
{
  m2::PointD m_finalPt;

public:
  virtual string GetName() const;
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback);
};


}
