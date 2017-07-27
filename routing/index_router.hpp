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
  class BestEdgeComparator final
  {
  public:
    BestEdgeComparator(m2::PointD const & point, m2::PointD const & direction);

    /// \returns true if |edge1| is closer to |m_point|, |m_direction| than |edge2|.
    bool Compare(Edge const & edge1, Edge const & edge2) const;

  private:
    /// \returns true if |edge| is almost parallel to vector |m_direction|.
    /// \note According to current implementation vectors |edge| and |m_direction|
    /// are almost parallel if angle between them less than 14 degrees.
    bool IsAlmostParallel(Edge const & edge) const;

    /// \returns the square of shortest distance from |m_point| to |edge| in mercator.
    double GetSquaredDist(Edge const & edge) const;

    m2::PointD const m_point;
    m2::PointD const m_direction;
  };

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
                                        vector<Segment> & subroute, m2::PointD & startPoint);
  IRouter::ResultCode AdjustRoute(Checkpoints const & checkpoints,
                                  m2::PointD const & startDirection,
                                  RouterDelegate const & delegate, Route & route);

  WorldGraph MakeWorldGraph();

  /// \brief Finds the best segment (edge) which may be considered as the start of the finish of the route.
  /// According to current implementation if a segment is near |point| and is almost parallel
  /// to |direction| vector, the segment will be better than others. If there's no an an almost parallel
  /// segment in neighbourhoods the closest segment will be chosen.
  /// \param isOutgoing == true is |point| is considered as the start of the route.
  /// isOutgoing == false is |point| is considered as the finish of the route.
  bool FindBestSegment(m2::PointD const & point, m2::PointD const & direction, bool isOutgoing,
                       WorldGraph & worldGraph, Segment & bestSegment) const;
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
