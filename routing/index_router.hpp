#pragma once

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/routing_result.hpp"

#include "routing/cross_mwm_graph.hpp"
#include "routing/directions_engine.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/fake_edges_container.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/joint.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/segmented_route.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/tree4d.hpp"

#include "std/unique_ptr.hpp"

#include <functional>
#include <set>
#include <string>
#include <vector>

class DataSource;

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

    /// \returns -1 if |edge1| is closer to |m_point| and |m_direction| than |edge2|.
    /// returns 0 if |edge1| and |edge2| have almost the same direction and are equidistant from |m_point|.
    /// returns 1 if |edge1| is further from |m_point| and |m_direction| than |edge2|.
    int Compare(Edge const & edge1, Edge const & edge2) const;

    bool IsDirectionValid() const { return !m_direction.IsAlmostZero(); }

    /// \brief According to current implementation vectors |edge| and |m_direction|
    /// are almost collinear and co-directional if the angle between them is less than 14 degrees.
    bool IsAlmostCodirectional(Edge const & edge) const;

  private:
    /// \returns the square of shortest distance from |m_point| to |edge| in mercator.
    double GetSquaredDist(Edge const & edge) const;

    m2::PointD const m_point;
    m2::PointD const m_direction;
  };

  IndexRouter(VehicleType vehicleType, bool loadAltitudes,
              CountryParentNameGetterFn const & countryParentNameGetterFn,
              TCountryFileFn const & countryFileFn, CourntryRectFn const & countryRectFn,
              shared_ptr<NumMwmIds> numMwmIds, unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
              traffic::TrafficCache const & trafficCache, DataSource & dataSource);

  std::unique_ptr<WorldGraph> MakeSingleMwmWorldGraph();
  bool FindBestSegment(m2::PointD const & point, m2::PointD const & direction,
                       bool isOutgoing, WorldGraph & worldGraph, Segment & bestSegment);

  // IRouter overrides:
  std::string GetName() const override { return m_name; }
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, RouterDelegate const & delegate,
                                  Route & route) override;

private:
  RouterResultCode DoCalculateRoute(Checkpoints const & checkpoints,
                                    m2::PointD const & startDirection,
                                    RouterDelegate const & delegate, Route & route);
  RouterResultCode CalculateSubroute(Checkpoints const & checkpoints, size_t subrouteIdx,
                                     RouterDelegate const & delegate, IndexGraphStarter & graph,
                                     std::vector<Segment> & subroute);

  RouterResultCode AdjustRoute(Checkpoints const & checkpoints,
                               m2::PointD const & startDirection,
                               RouterDelegate const & delegate, Route & route);

  std::unique_ptr<WorldGraph> MakeWorldGraph();

  /// \brief Finds the best segment (edge) which may be considered as the start of the finish of the route.
  /// According to current implementation if a segment is near |point| and is almost codirectional
  /// to |direction|, the segment will be better than others. If there's no almost codirectional
  /// segment in the neighbourhood then the closest segment to |point| will be chosen.
  /// \param isOutgoing == true if |point| is considered as the start of the route.
  /// isOutgoing == false if |point| is considered as the finish of the route.
  /// \param bestSegmentIsAlmostCodirectional is filled with true if |bestSegment| is chosen
  /// because |direction| and direction of |bestSegment| are almost equal and with false otherwise.
  /// \return true if the best segment is found and false otherwise.
  bool FindBestSegment(m2::PointD const & point, m2::PointD const & direction, bool isOutgoing,
                       WorldGraph & worldGraph, Segment & bestSegment,
                       bool & bestSegmentIsAlmostCodirectional) const;

  // Input route may contains 'leaps': shortcut edges from mwm border enter to exit.
  // ProcessLeaps replaces each leap with calculated route through mwm.
  RouterResultCode ProcessLeaps(std::vector<Segment> const & input,
                                   RouterDelegate const & delegate, WorldGraph::Mode prevMode,
                                   IndexGraphStarter & starter, std::vector<Segment> & output);
  RouterResultCode RedressRoute(std::vector<Segment> const & segments,
                                RouterDelegate const & delegate, IndexGraphStarter & starter,
                                Route & route) const;

  bool AreMwmsNear(std::set<NumMwmId> const & mwmIds) const;
  bool DoesTransitSectionExist(NumMwmId numMwmId) const;
  RouterResultCode ConvertTransitResult(std::set<NumMwmId> const & mwmIds,
                                        RouterResultCode resultCode) const;

  template <typename Graph>
  RouterResultCode ConvertResult(typename AStarAlgorithm<Graph>::Result result) const
  {
    switch (result)
    {
    case AStarAlgorithm<Graph>::Result::NoPath: return RouterResultCode::RouteNotFound;
    case AStarAlgorithm<Graph>::Result::Cancelled: return RouterResultCode::Cancelled;
    case AStarAlgorithm<Graph>::Result::OK: return RouterResultCode::NoError;
    }
    UNREACHABLE();
  }

  template <typename Graph>
  RouterResultCode FindPath(
      typename AStarAlgorithm<Graph>::Params & params, std::set<NumMwmId> const & mwmIds,
      RoutingResult<typename Graph::Vertex, typename Graph::Weight> & routingResult) const
  {
    AStarAlgorithm<Graph> algorithm;
    if (params.m_graph.GetMode() == WorldGraph::Mode::LeapsOnly)
    {
      return ConvertTransitResult(mwmIds,
                                  ConvertResult<Graph>(algorithm.FindPath(params, routingResult)));
    }
    return ConvertTransitResult(
        mwmIds, ConvertResult<Graph>(algorithm.FindPathBidirectional(params, routingResult)));
  }

  VehicleType m_vehicleType;
  bool m_loadAltitudes;
  std::string const m_name;
  DataSource & m_dataSource;
  std::shared_ptr<VehicleModelFactoryInterface> m_vehicleModelFactory;

  TCountryFileFn const m_countryFileFn;
  CourntryRectFn const m_countryRectFn;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::shared_ptr<m4::Tree<NumMwmId>> m_numMwmTree;
  std::shared_ptr<TrafficStash> m_trafficStash;
  FeaturesRoadGraph m_roadGraph;

  std::shared_ptr<EdgeEstimator> m_estimator;
  std::unique_ptr<IDirectionsEngine> m_directionsEngine;
  std::unique_ptr<SegmentedRoute> m_lastRoute;
  std::unique_ptr<FakeEdgesContainer> m_lastFakeEdges;
};
}  // namespace routing
