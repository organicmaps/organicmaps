#pragma once

#include "routing/cross_mwm_graph.hpp"
#include "routing/directions_engine.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/joint.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/router.hpp"
#include "routing/routing_mapping.hpp"
#include "routing/segmented_route.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/tree4d.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexGraph;
class IndexGraphStarter;

class IndexRouter : public IRouter
{
public:
  IndexRouter(string const & name, TCountryFileFn const & countryFileFn,
              CourntryRectFn const & countryRectFn, shared_ptr<NumMwmIds> numMwmIds,
              unique_ptr<m4::Tree<NumMwmId>> numMwmTree, shared_ptr<TrafficStash> trafficStash,
              shared_ptr<VehicleModelFactory> vehicleModelFactory,
              shared_ptr<EdgeEstimator> estimator, unique_ptr<IDirectionsEngine> directionsEngine,
              Index & index);

  // IRouter overrides:
  virtual string GetName() const override { return m_name; }
  virtual ResultCode CalculateRoute(Checkpoints const & checkpoints,
                                    m2::PointD const & startDirection, bool adjustToPrevRoute,
                                    RouterDelegate const & delegate, Route & route) override;

  /// \note |numMwmIds| should not be null.
  static unique_ptr<IndexRouter> CreateCarRouter(TCountryFileFn const & countryFileFn,
                                                 CourntryRectFn const & coutryRectFn,
                                                 shared_ptr<NumMwmIds> numMwmIds,
                                                 unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                                                 traffic::TrafficCache const & trafficCache,
                                                 Index & index);

private:
  IRouter::ResultCode DoCalculateRoute(Checkpoints const & checkpoints,
                                       m2::PointD const & startDirection,
                                       RouterDelegate const & delegate, Route & route);
  IRouter::ResultCode CalculateSubroute(Checkpoints const & checkpoints, size_t subrouteIdx,
                                        Segment const & startSegment,
                                        RouterDelegate const & delegate, WorldGraph & graph,
                                        vector<Segment> & subroute);
  IRouter::ResultCode AdjustRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                                  m2::PointD const & finalPoint, RouterDelegate const & delegate,
                                  Route & route);

  WorldGraph MakeWorldGraph();

  /// \brief Finds closest edges which may be considered as start of finish of the route.
  /// \param isOutgoing == true is |point| is considered as the start of the route.
  /// isOutgoing == false is |point| is considered as the finish of the route.
  bool FindClosestSegment(m2::PointD const & point, bool isOutgoing, WorldGraph & worldGraph,
                          Segment & closestSegment) const;
  // Input route may contains 'leaps': shortcut edges from mwm border enter to exit.
  // ProcessLeaps replaces each leap with calculated route through mwm.
  IRouter::ResultCode ProcessLeaps(vector<Segment> const & input,
                                   RouterDelegate const & delegate,
                                   WorldGraph::Mode prevMode,
                                   IndexGraphStarter & starter,
                                   vector<Segment> & output);
  IRouter::ResultCode RedressRoute(vector<Segment> const & segments,
                                   RouterDelegate const & delegate, IndexGraphStarter & starter,
                                   Route & route) const;

  bool AreMwmsNear(NumMwmId startId, NumMwmId finishId) const;

  string const m_name;
  Index & m_index;
  TCountryFileFn const m_countryFileFn;
  CourntryRectFn const m_countryRectFn;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<m4::Tree<NumMwmId>> m_numMwmTree;
  shared_ptr<TrafficStash> m_trafficStash;
  RoutingIndexManager m_indexManager;
  FeaturesRoadGraph m_roadGraph;
  shared_ptr<VehicleModelFactory> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;
  unique_ptr<IDirectionsEngine> m_directionsEngine;
  unique_ptr<SegmentedRoute> m_lastRoute;
};
}  // namespace routing
