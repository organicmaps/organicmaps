#pragma once

#include "async_router.hpp"
#include "road_graph.hpp"

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"
#include "../std/scoped_ptr.hpp"


class Index;

namespace routing
{

class IVehicleModel;

class RoadGraphRouter : public AsyncRouter
{
public:
  RoadGraphRouter(Index const * pIndex);
  ~RoadGraphRouter();

  virtual void CalculateRouteOnMwm(vector<RoadPos> const & startPos, vector<RoadPos> const & finalPos, vector<RoadPos> & route) = 0;
  virtual void SetRoadGraph(IRoadGraph * pRoadGraph) { m_pRoadGraph.reset(pRoadGraph); }

  virtual ResultCode CalculateRouteImpl(m2::PointD const & startPt,
                                        m2::PointD const & startDr,
                                        m2::PointD const & finalPt,
                                        CancelFlagT const & requestCancel,
                                        Route & route);

protected:
  size_t GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos);
  bool IsMyMWM(size_t mwmID) const;

  scoped_ptr<IRoadGraph> m_pRoadGraph;
  scoped_ptr<IVehicleModel> m_vehicleModel;
  Index const * m_pIndex;
};

} // namespace routing
