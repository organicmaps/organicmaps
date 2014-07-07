#include "helicopter_router.hpp"
#include "route.hpp"

#include "../base/macros.hpp"

namespace routing
{

void HelicopterRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  m_finalPt = finalPt;
}

void HelicopterRouter::CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback)
{
  m2::PointD points[] = {startingPt, m_finalPt};
  Route route(vector<m2::PointD>(&points[0], &points[0] + ARRAY_SIZE(points)));
  callback(route);
}

}
