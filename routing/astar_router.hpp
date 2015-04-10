#pragma once

#include "road_graph_router.hpp"
#include "../std/map.hpp"
#include "../std/queue.hpp"


namespace routing
{

class AStarRouter : public RoadGraphRouter
{
  typedef RoadGraphRouter BaseT;
public:
  AStarRouter(Index const * pIndex = 0) : BaseT(pIndex) {}

  // IRouter overrides:
  string GetName() const override { return "astar-pedestrian"; }

  // RoadGraphRouter overrides:
  ResultCode CalculateRouteM2M(vector<RoadPos> const & startPos, vector<RoadPos> const & finalPos,
                               vector<RoadPos> & route) override;

protected:

  typedef map<RoadPos, double> RoadPosToDoubleMapT;
  typedef map<RoadPos, RoadPos> RoadPosParentMapT;

  double HeuristicCostEstimate(RoadPos const & v, vector<RoadPos> const & goals);
  double DistanceBetween(RoadPos const & v1, RoadPos const & v2);

};


}
