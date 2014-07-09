#pragma once

#include "router.hpp"
#include "road_graph.hpp"

#include "../geometry/point2d.hpp"
#include "../std/vector.hpp"

namespace routing
{

class IRoadGraph;

class RoadGraphRouter : public IRouter
{
public:
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback);

  virtual void SetFinalRoadPos(vector<RoadPos> const & finalPos) = 0;
  virtual void CalculateRoute(vector<RoadPos> const & startPos, vector<RoadPos> & route) = 0;
  virtual void SetRoadGraph(IRoadGraph * pRoadGraph) { m_pRoadGraph = pRoadGraph; }

protected:
  IRoadGraph * m_pRoadGraph;
};

} // namespace routing
