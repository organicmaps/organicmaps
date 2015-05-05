#pragma once

#include "routing/road_graph.hpp"
#include "routing/router.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Index;

namespace routing
{

class RoadGraphRouter : public IRouter
{
public:
  typedef function<string(m2::PointD const &)> TMwmFileByPointFn;

  RoadGraphRouter(Index const * pIndex, unique_ptr<IVehicleModel> && vehicleModel,
                  TMwmFileByPointFn const & fn);
  ~RoadGraphRouter();

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;

  virtual void SetRoadGraph(unique_ptr<IRoadGraph> && roadGraph) { m_roadGraph = move(roadGraph); }
  inline IRoadGraph * GetGraph() { return m_roadGraph.get(); }

protected:
  virtual ResultCode CalculateRoute(Junction const & startPos, Junction const & finalPos,
                                    vector<Junction> & route) = 0;

private:
  /// @todo This method fits better in features_road_graph.
  void GetClosestEdges(m2::PointD const & pt, vector<pair<Edge, m2::PointD>> & edges);

  bool IsMyMWM(MwmSet::MwmId const & mwmID) const;

  unique_ptr<IRoadGraph> m_roadGraph;
  unique_ptr<IVehicleModel> const m_vehicleModel;
  Index const * const m_pIndex; // non-owning ptr
  TMwmFileByPointFn const m_countryFileFn;
};
  
}  // namespace routing
