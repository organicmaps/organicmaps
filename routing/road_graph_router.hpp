#pragma once

#include "router.hpp"
#include "road_graph.hpp"

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"
#include "../std/scoped_ptr.hpp"


class Index;

namespace routing
{

class IVehicleModel;

class RoadGraphRouter : public IRouter
{
public:
  RoadGraphRouter(Index const * pIndex);
  ~RoadGraphRouter();

  virtual void CalculateM2MRoute(vector<RoadPos> const & startPos, vector<RoadPos> const & finalPos,
                                 vector<RoadPos> & route) = 0;
  virtual void SetRoadGraph(IRoadGraph * pRoadGraph) { m_pRoadGraph.reset(pRoadGraph); }

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;

protected:
  size_t GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos);
  bool IsMyMWM(size_t mwmID) const;

  scoped_ptr<IRoadGraph> m_pRoadGraph;
  scoped_ptr<IVehicleModel> m_vehicleModel;
  Index const * m_pIndex;
};

} // namespace routing
