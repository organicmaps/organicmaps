#pragma once

#include "routing/road_graph.hpp"
#include "routing/router.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Index;

namespace routing
{
typedef function<string (m2::PointD const &)> CountryFileFnT;

class RoadGraphRouter : public IRouter
{
public:
  RoadGraphRouter(Index const * pIndex, unique_ptr<IVehicleModel> && vehicleModel, CountryFileFnT const & fn);
  ~RoadGraphRouter();

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;
  virtual ResultCode CalculateRoute(RoadPos const & startPos, RoadPos const & finalPos,
                                    vector<RoadPos> & route) = 0;
  virtual void SetRoadGraph(unique_ptr<IRoadGraph> && roadGraph) { m_roadGraph = move(roadGraph); }
  inline IRoadGraph * GetGraph() { return m_roadGraph.get(); }

protected:
  /// @todo This method fits better in features_road_graph.
  void GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos);

  bool IsMyMWM(MwmSet::MwmId const & mwmID) const;

  unique_ptr<IRoadGraph> m_roadGraph;
  unique_ptr<IVehicleModel> const m_vehicleModel;
  Index const * m_pIndex;  // non-owning ptr
  CountryFileFnT m_countryFileFn;
};

}  // namespace routing
