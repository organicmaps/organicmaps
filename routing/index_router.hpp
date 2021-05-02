#pragma once

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/base/routing_result.hpp"
#include "routing/cross_mwm_graph.hpp"
#include "routing/directions_engine.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/fake_edges_container.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/guides_connections.hpp"
#include "routing/index_graph_starter_joints.hpp"
#include "routing/joint.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/segment.hpp"
#include "routing/segmented_route.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
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

    /// \returns the square of shortest distance from |m_point| to |edge| in mercator.
    double GetSquaredDist(Edge const & edge) const;

  private:
    m2::PointD const m_point;
    m2::PointD const m_direction;
  };

  IndexRouter(VehicleType vehicleType, bool loadAltitudes,
              CountryParentNameGetterFn const & countryParentNameGetterFn,
              TCountryFileFn const & countryFileFn, CourntryRectFn const & countryRectFn,
              std::shared_ptr<NumMwmIds> numMwmIds, std::unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
              traffic::TrafficCache const & trafficCache, DataSource & dataSource);

  std::unique_ptr<WorldGraph> MakeSingleMwmWorldGraph();
  bool FindBestSegments(m2::PointD const & checkpoint, m2::PointD const & direction, bool isOutgoing,
                        WorldGraph & worldGraph, std::vector<Segment> & bestSegments);
  bool FindBestEdges(m2::PointD const & checkpoint,
                     platform::CountryFile const & pointCountryFile,
                     m2::PointD const & direction, bool isOutgoing,
                     double closestEdgesRadiusM, WorldGraph & worldGraph,
                     std::vector<Edge> & bestEdges,
                     bool & bestSegmentIsAlmostCodirectional) const;

  // IRouter overrides:
  std::string GetName() const override { return m_name; }
  void ClearState() override;

  void SetGuides(GuidesTracks && guides) override;
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints,
                                  m2::PointD const & startDirection, bool adjustToPrevRoute,
                                  RouterDelegate const & delegate, Route & route) override;

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction,
                                   double radius, EdgeProj & proj) override;

  VehicleType GetVehicleType() const { return m_vehicleType; }

private:
  RouterResultCode CalculateSubrouteJointsMode(IndexGraphStarter & starter,
                                               RouterDelegate const & delegate,
                                               std::shared_ptr<AStarProgress> const & progress,
                                               std::vector<Segment> & subroute);
  RouterResultCode CalculateSubrouteNoLeapsMode(IndexGraphStarter & starter,
                                                RouterDelegate const & delegate,
                                                std::shared_ptr<AStarProgress> const & progress,
                                                std::vector<Segment> & subroute);
  RouterResultCode CalculateSubrouteLeapsOnlyMode(Checkpoints const & checkpoints,
                                                  size_t subrouteIdx, IndexGraphStarter & starter,
                                                  RouterDelegate const & delegate,
                                                  std::shared_ptr<AStarProgress> const & progress,
                                                  std::vector<Segment> & subroute);

  RouterResultCode DoCalculateRoute(Checkpoints const & checkpoints,
                                    m2::PointD const & startDirection,
                                    RouterDelegate const & delegate, Route & route);
  RouterResultCode CalculateSubroute(Checkpoints const & checkpoints, size_t subrouteIdx,
                                     RouterDelegate const & delegate,
                                     std::shared_ptr<AStarProgress> const & progress,
                                     IndexGraphStarter & graph, std::vector<Segment> & subroute,
                                     bool guidesActive = false);

  RouterResultCode AdjustRoute(Checkpoints const & checkpoints,
                               m2::PointD const & startDirection,
                               RouterDelegate const & delegate, Route & route);

  std::unique_ptr<WorldGraph> MakeWorldGraph();

  /// \brief Removes all roads from |roads| which goes to dead ends and all road which
  /// is not good according to |worldGraph|. For car routing there are roads with hwtag nocar as well.
  /// \param checkpoint which is used to look for the closest segment in a road. The closest segment
  /// is used then to check if it's a dead end.
  void EraseIfDeadEnd(WorldGraph & worldGraph, m2::PointD const & checkpoint,
                      std::vector<IRoadGraph::FullRoadInfo> & roads) const;

  /// \returns true if a segment (|point|, |edgeProjection.second|) crosses one of segments
  /// in |fences| except for a one which has the same geometry with |edgeProjection.first|.
  bool IsFencedOff(m2::PointD const & point,
                   std::pair<Edge, geometry::PointWithAltitude> const & edgeProjection,
                   std::vector<IRoadGraph::FullRoadInfo> const & fences) const;

  void RoadsToNearestEdges(
      m2::PointD const & point, std::vector<IRoadGraph::FullRoadInfo> const & roads,
      IsEdgeProjGood const & isGood,
      std::vector<std::pair<Edge, geometry::PointWithAltitude>> & edgeProj) const;

  Segment GetSegmentByEdge(Edge const & edge) const;

  /// \brief Fills |closestCodirectionalEdge| with a codirectional edge which is closest to
  /// |point| and returns true if there's any. If not returns false.
  bool FindClosestCodirectionalEdge(
      m2::PointD const & point, m2::PointD const & direction,
      std::vector<std::pair<Edge, geometry::PointWithAltitude>> const & candidates,
      Edge & closestCodirectionalEdge) const;

  /// \brief Finds the best segments (edges) which may be considered as starts or finishes
  /// of the route. According to current implementation the closest to |checkpoint| segment which
  /// is almost codirectianal to |direction| is the best.
  /// If there's no an almost codirectional segment in the neighbourhood then all not dead end
  /// candidates which may be reached without crossing road graph will be added to |bestSegments|.
  /// \param isOutgoing == true if |checkpoint| is considered as the start of the route.
  /// isOutgoing == false if |checkpoint| is considered as the finish of the route.
  /// \param bestSegmentIsAlmostCodirectional is filled with true if |bestSegment| is chosen
  /// because |direction| and direction of |bestSegment| are almost equal and with false otherwise.
  /// \return true if the best segment is found and false otherwise.
  /// \note Candidates in |bestSegments| are sorted from better to worse.
  bool FindBestSegments(m2::PointD const & checkpoint, m2::PointD const & direction, bool isOutgoing,
                        WorldGraph & worldGraph, std::vector<Segment> & bestSegments,
                        bool & bestSegmentIsAlmostCodirectional) const;

  // Input route may contains 'leaps': shortcut edges from mwm border enter to exit.
  // ProcessLeaps replaces each leap with calculated route through mwm.
  RouterResultCode ProcessLeapsJoints(std::vector<Segment> const & input,
                                      RouterDelegate const & delegate, WorldGraphMode prevMode,
                                      IndexGraphStarter & starter,
                                      std::shared_ptr<AStarProgress> const & progress,
                                      std::vector<Segment> & output);
  RouterResultCode RedressRoute(std::vector<Segment> const & segments,
                                base::Cancellable const & cancellable, IndexGraphStarter & starter,
                                Route & route) const;

  bool AreMwmsNear(IndexGraphStarter const & starter) const;
  bool DoesTransitSectionExist(NumMwmId numMwmId) const;

  RouterResultCode ConvertTransitResult(std::set<NumMwmId> const & mwmIds,
                                        RouterResultCode resultCode) const;

  /// \brief Fills |speedcamProhibitedMwms| with mwms which are crossed by |segments|
  /// where speed cameras are prohibited.
  void FillSpeedCamProhibitedMwms(std::vector<Segment> const & segments,
                                  std::vector<platform::CountryFile> & speedCamProhibitedMwms) const;

  template <typename Vertex, typename Edge, typename Weight>
  RouterResultCode ConvertResult(typename AStarAlgorithm<Vertex, Edge, Weight>::Result result) const
  {
    switch (result)
    {
    case AStarAlgorithm<Vertex, Edge, Weight>::Result::NoPath: return RouterResultCode::RouteNotFound;
    case AStarAlgorithm<Vertex, Edge, Weight>::Result::Cancelled: return RouterResultCode::Cancelled;
    case AStarAlgorithm<Vertex, Edge, Weight>::Result::OK: return RouterResultCode::NoError;
    }
    UNREACHABLE();
  }

  template <typename Vertex, typename Edge, typename Weight, typename AStarParams>
  RouterResultCode FindPath(AStarParams & params, std::set<NumMwmId> const & mwmIds,
                            RoutingResult<Vertex, Weight> & routingResult) const
  {
    AStarAlgorithm<Vertex, Edge, Weight> algorithm;
    return ConvertTransitResult(
        mwmIds, ConvertResult<Vertex, Edge, Weight>(algorithm.FindPathBidirectional(params, routingResult)));
  }

  void SetupAlgorithmMode(IndexGraphStarter & starter, bool guidesActive = false) const;
  uint32_t ConnectTracksOnGuidesToOsm(std::vector<m2::PointD> const & checkpoints,
                                      WorldGraph & graph);

  void ConnectCheckpointsOnGuidesToOsm(std::vector<m2::PointD> const & checkpoints,
                                       WorldGraph & graph);

  void AddGuidesOsmConnectionsToGraphStarter(size_t checkpointIdxFrom, size_t checkpointIdxTo,
                                             IndexGraphStarter & starter);

  void AppendPartsOfReal(LatLonWithAltitude const & point1, LatLonWithAltitude const & point2,
                         uint32_t & startIdx, ConnectionToOsm & link);

  std::vector<Segment> GetBestSegments(m2::PointD const & checkpoint, WorldGraph & graph);

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
  std::unique_ptr<DirectionsEngine> m_directionsEngine;
  std::unique_ptr<SegmentedRoute> m_lastRoute;
  std::unique_ptr<FakeEdgesContainer> m_lastFakeEdges;

  // If a ckeckpoint is near to the guide track we need to build route through this track.
  GuidesConnections m_guides;

  CountryParentNameGetterFn m_countryParentNameGetterFn;
};
}  // namespace routing
