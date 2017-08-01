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

#include <functional>
#include <memory>
#include <string>
#include <vector>

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
    using IsOnewayFn = std::function<bool(FeatureID const & featureId)>;

    BestEdgeComparator(m2::PointD const & point,  m2::PointD const & direction, IsOnewayFn const & isOnewayFn);

    /// \returns true if |edge1| is closer to |m_point| and |m_direction| than |edge2|.
    bool Compare(Edge const & edge1, Edge const & edge2) const;

    /// \returns true if |edge| has a oneway feature and if |edge| is almost parallel to vector |m_direction|.
    /// returns true if |edge| has a twoway feature and if |edge| is almost collinear to vector |m_direction|.
    /// returns false otherwise.
    bool IsAlmostConformed(Edge const & edge) const;

    bool IsDirectionValid() const { return !m_direction.IsAlmostZero(); }

    /// \brief According to current implementation vectors |edge| and |m_direction|
    /// are almost collinear if angle between line of the vectors less than 14 degrees.
    /// If also the angle between the vectors is less than 14 degrees |forward| will be set to true.
    /// If the vectors have almost opposite direction |forward| will be set to false.
    bool IsAlmostCollinear(Edge const & edge, bool & forward) const;

  private:
    /// \returns the square of shortest distance from |m_point| to |edge| in mercator.
    double GetSquaredDist(Edge const & edge) const;

    m2::PointD const m_point;
    m2::PointD const m_direction;
    IsOnewayFn const m_isOnewayFn;
  };


  IndexRouter(std::string const & name, TCountryFileFn const & countryFileFn,
              CourntryRectFn const & countryRectFn, std::shared_ptr<NumMwmIds> numMwmIds,
              std::unique_ptr<m4::Tree<NumMwmId>> numMwmTree, std::shared_ptr<TrafficStash> trafficStash,
              VehicleType vehicleType, std::shared_ptr<VehicleModelFactory> vehicleModelFactory,
              std::shared_ptr<EdgeEstimator> estimator, std::unique_ptr<IDirectionsEngine> directionsEngine,
              Index & index);

  // IRouter overrides:
  std::string GetName() const override { return m_name; }
  ResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                            bool adjustToPrevRoute, RouterDelegate const & delegate,
                            Route & route) override;

  static std::unique_ptr<IndexRouter> Create(VehicleType vehicleType,
                                             TCountryFileFn const & countryFileFn,
                                             CourntryRectFn const & coutryRectFn,
                                             std::shared_ptr<NumMwmIds> numMwmIds,
                                             std::unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                                             traffic::TrafficCache const & trafficCache, Index & index);

private:
  IRouter::ResultCode DoCalculateRoute(Checkpoints const & checkpoints,
                                       m2::PointD const & startDirection,
                                       RouterDelegate const & delegate, Route & route);
  IRouter::ResultCode CalculateSubroute(Checkpoints const & checkpoints, size_t subrouteIdx,
                                        Segment const & startSegment, bool startSegmentIsAlmostParallelDirection,
                                        RouterDelegate const & delegate, WorldGraph & graph,
                                        std::vector<Segment> & subroute, Junction & startJunction);

  IRouter::ResultCode AdjustRoute(Checkpoints const & checkpoints,
                                  m2::PointD const & startDirection,
                                  RouterDelegate const & delegate, Route & route);

  WorldGraph MakeWorldGraph();

  /// \brief Finds the best segment (edge) which may be considered as the start of the finish of the route.
  /// According to current implementation if a segment is near |point| and is almost parallel
  /// to |direction| vector, the segment will be better than others. If there's no an an almost parallel
  /// segment in neighbourhoods the closest segment to |point| will be chosen.
  /// \param isOutgoing == true is |point| is considered as the start of the route.
  /// isOutgoing == false is |point| is considered as the finish of the route.
  /// \param bestSegmentIsAlmostConformDirection is filled with true if |bestSegment| is chosen because
  /// vector |direction| and vector of |bestSegment| are almost equal and with false otherwise.
  bool FindBestSegment(m2::PointD const & point, m2::PointD const & direction, bool isOutgoing,
                       WorldGraph & worldGraph, Segment & bestSegment,
                       bool & bestSegmentIsAlmostConformDirection) const;
  // Input route may contains 'leaps': shortcut edges from mwm border enter to exit.
  // ProcessLeaps replaces each leap with calculated route through mwm.
  IRouter::ResultCode ProcessLeaps(std::vector<Segment> const & input,
                                   RouterDelegate const & delegate,
                                   WorldGraph::Mode prevMode,
                                   IndexGraphStarter & starter,
                                   std::vector<Segment> & output);
  IRouter::ResultCode RedressRoute(std::vector<Segment> const & segments,
                                   RouterDelegate const & delegate, IndexGraphStarter & starter,
                                   Route & route) const;

  bool AreMwmsNear(NumMwmId startId, NumMwmId finishId) const;
  bool IsOneWayFeature(FeatureID const & featureId) const;

  std::string const m_name;
  Index & m_index;
  TCountryFileFn const m_countryFileFn;
  CourntryRectFn const m_countryRectFn;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::shared_ptr<m4::Tree<NumMwmId>> m_numMwmTree;
  std::shared_ptr<TrafficStash> m_trafficStash;
  RoutingIndexManager m_indexManager;
  FeaturesRoadGraph m_roadGraph;

  VehicleType m_vehicleType;
  std::shared_ptr<VehicleModelFactory> m_vehicleModelFactory;
  std::shared_ptr<EdgeEstimator> m_estimator;
  std::unique_ptr<IDirectionsEngine> m_directionsEngine;
  std::unique_ptr<SegmentedRoute> m_lastRoute;
};
}  // namespace routing
