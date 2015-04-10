#pragma once

#include "road_graph.hpp"
#include "router.hpp"
#include "vehicle_model.hpp"

#include "../geometry/point2d.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"

class Index;

namespace routing
{
class RoadGraphRouter : public IRouter
{
public:
  RoadGraphRouter(Index const * pIndex, unique_ptr<IVehicleModel> && vehicleModel);
  ~RoadGraphRouter();

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;
  virtual ResultCode CalculateRouteM2M(vector<RoadPos> const & startPos,
                                       vector<RoadPos> const & finalPos,
                                       vector<RoadPos> & route) = 0;
  virtual void SetRoadGraph(unique_ptr<IRoadGraph> && roadGraph) { m_roadGraph = move(roadGraph); }
  inline IRoadGraph * GetGraph() { return m_roadGraph.get(); }

protected:
  size_t GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos);
  bool IsMyMWM(size_t mwmID) const;

  unique_ptr<IRoadGraph> m_roadGraph;
  unique_ptr<IVehicleModel> const m_vehicleModel;
  Index const * m_pIndex;  // non-owning ptr
};

} // namespace routing
