#pragma once

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/base/routing_result.hpp"

#include "routing/data_source.hpp"
#include "routing/directions_engine.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/fake_edges_container.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/guides_connections.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/regions_decl.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/segment.hpp"
#include "routing/segmented_route.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "platform/country_file.hpp"

#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace traffic
{
class TrafficCache;
}

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

  IndexRouter(VehicleType vehicleType, bool loadAltitudes, CountryParentNameGetterFn const & countryParentNameGetterFn,
              TCountryFileFn const & countryFileFn, CountryRectFn const & countryRectFn,
              std::shared_ptr<NumMwmIds> numMwmIds, std::unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
              traffic::TrafficCache const & trafficCache, DataSource & dataSource);

  std::unique_ptr<WorldGraph> MakeSingleMwmWorldGraph();

  // IRouter overrides:
  std::string GetName() const override { return m_name; }
  void ClearState() override;

  void SetGuides(GuidesTracks && guides) override;
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, RouterDelegate const & delegate, Route & route) override;

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override;

  bool GetBestOutgoingEdges(m2::PointD const & checkpoint, WorldGraph & graph, std::vector<Edge> & edges);

  VehicleType GetVehicleType() const { return m_vehicleType; }

private:
  RouterResultCode CalculateSubrouteJointsMode(IndexGraphStarter & starter, RouterDelegate const & delegate,
                                               std::shared_ptr<AStarProgress> const & progress,
                                               std::vector<Segment> & subroute);
  RouterResultCode CalculateSubrouteNoLeapsMode(IndexGraphStarter & starter, RouterDelegate const & delegate,
                                                std::shared_ptr<AStarProgress> const & progress,
                                                std::vector<Segment> & subroute);
  RouterResultCode CalculateSubrouteLeapsOnlyMode(Checkpoints const & checkpoints, size_t subrouteIdx,
                                                  IndexGraphStarter & starter, RouterDelegate const & delegate,
                                                  std::shared_ptr<AStarProgress> const & progress,
                                                  std::vector<Segment> & subroute);

  RouterResultCode DoCalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                    RouterDelegate const & delegate, Route & route);
  RouterResultCode CalculateSubroute(Checkpoints const & checkpoints, size_t subrouteIdx,
                                     RouterDelegate const & delegate, std::shared_ptr<AStarProgress> const & progress,
                                     IndexGraphStarter & graph, std::vector<Segment> & subroute,
                                     bool guidesActive = false);

  RouterResultCode AdjustRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                               RouterDelegate const & delegate, Route & route);

  std::unique_ptr<WorldGraph> MakeWorldGraph();

  using EdgeProjectionT = IRoadGraph::EdgeProjectionT;
  class PointsOnEdgesSnapping
  {
    IndexRouter & m_router;
    WorldGraph & m_graph;

    using RoadInfoT = IRoadGraph::FullRoadInfo;
    using EdgeProjectionT = IndexRouter::EdgeProjectionT;

    // The idea here is to store dead-ends from the successful previous call, not to filter
    // dead-end candidates if they belong to one graph's cluster (island).
    std::set<Segment> m_deadEnds[2];
    std::vector<Segment> m_startSegments;

    static uint32_t constexpr kFirstSearchDistanceM = 40;

  public:
    PointsOnEdgesSnapping(IndexRouter & router, WorldGraph & graph) : m_router(router), m_graph(graph) {}

    /// @return 0 - ok, 1 - start not found, 2 - finish not found.
    int Snap(m2::PointD const & start, m2::PointD const & finish, m2::PointD const & direction,
             FakeEnding & startEnding, FakeEnding & finishEnding, bool & startIsCodirectional);

    void SetNextStartSegment(Segment const & seg) { m_startSegments = {seg}; }

  private:
    void FillDeadEndsCache(m2::PointD const & point);

    /// \brief Removes all roads from |roads| that go to dead ends and all roads that
    /// are not good according to |worldGraph|. For car routing there are roads with hwtag nocar as well.
    /// \param checkpoint which is used to look for the closest segment in a road. The closest segment
    /// is used then to check if it's a dead end.
    void EraseIfDeadEnd(m2::PointD const & checkpoint, std::vector<RoadInfoT> & roads,
                        std::set<Segment> & deadEnds) const;

    /// \returns true if a segment (|point|, |edgeProjection.second|) crosses one of segments
    /// in |fences| except for the one which has the same geometry with |edgeProjection.first|.
    static bool IsFencedOff(m2::PointD const & point, EdgeProjectionT const & edgeProjection,
                            std::vector<RoadInfoT> const & fences);

    static void RoadsToNearestEdges(m2::PointD const & point, std::vector<RoadInfoT> const & roads,
                                    IsEdgeProjGood const & isGood, std::vector<EdgeProjectionT> & edgeProj);

    Segment GetSegmentByEdge(Edge const & edge) const;

  public:
    /// \brief Fills |closestCodirectionalEdge| with a codirectional edge which is closest to
    /// |point| and returns true if there's any. If not returns false.
    static bool FindClosestCodirectionalEdge(m2::PointD const & point, m2::PointD const & direction,
                                             std::vector<EdgeProjectionT> const & candidates,
                                             Edge & closestCodirectionalEdge);

    /// \brief Finds best segments (edges) that may be considered as starts or finishes
    /// of the route. According to current implementation the closest to |checkpoint| segment which
    /// is almost codirectianal to |direction| is the best.
    /// If there's no an almost codirectional segment in the neighbourhood then all not dead end
    /// candidates that may be reached without crossing the road graph will be added to |bestSegments|.
    /// \param isOutgoing == true if |checkpoint| is considered as the start of the route.
    /// isOutgoing == false if |checkpoint| is considered as the finish of the route.
    /// \param bestSegmentIsAlmostCodirectional is filled with true if |bestSegment| is chosen
    /// because |direction| and direction of |bestSegment| are almost equal and with false otherwise.
    /// \return true if the best segment is found and false otherwise.
    /// \note Candidates in |bestSegments| are sorted from better to worse.
    bool FindBestSegments(m2::PointD const & checkpoint, m2::PointD const & direction, bool isOutgoing,
                          std::vector<Segment> & bestSegments, bool & bestSegmentIsAlmostCodirectional);

    bool FindBestEdges(m2::PointD const & checkpoint, m2::PointD const & direction, bool isOutgoing,
                       double closestEdgesRadiusM, std::vector<Edge> & bestEdges,
                       bool & bestSegmentIsAlmostCodirectional);
  };

  using RoutingResultT = RoutingResult<Segment, RouteWeight>;
  class RoutesCalculator
  {
    std::map<std::pair<Segment, Segment>, RoutingResultT> m_cache;
    IndexGraphStarter & m_starter;
    RouterDelegate const & m_delegate;

  public:
    RoutesCalculator(IndexGraphStarter & starter, RouterDelegate const & delegate)
      : m_starter(starter)
      , m_delegate(delegate)
    {}

    using ProgressPtrT = std::shared_ptr<AStarProgress>;
    RoutingResultT const * Calc(Segment const & beg, Segment const & end, ProgressPtrT const & progress,
                                double progressCoef);
    // Makes JointSingleMwm first and Joints then, if first attempt was failed.
    RoutingResultT const * Calc2Times(Segment const & beg, Segment const & end, ProgressPtrT const & progress,
                                      double progressCoef);
  };

  // Input route may contains 'leaps': shortcut edges from mwm border enter to exit.
  // ProcessLeaps replaces each leap with calculated route through mwm.
  RouterResultCode ProcessLeapsJoints(std::vector<Segment> const & input, IndexGraphStarter & starter,
                                      std::shared_ptr<AStarProgress> const & progress, RoutesCalculator & calculator,
                                      RoutingResultT & result);

  RouterResultCode RedressRoute(std::vector<Segment> const & segments, base::Cancellable const & cancellable,
                                IndexGraphStarter & starter, Route & route);

  bool AreSpeedCamerasProhibited(NumMwmId mwmID) const;
  bool AreMwmsNear(IndexGraphStarter const & starter) const;
  bool DoesTransitSectionExist(NumMwmId numMwmId);

  RouterResultCode ConvertTransitResult(std::set<NumMwmId> const & mwmIds, RouterResultCode resultCode);

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
                            RoutingResult<Vertex, Weight> & routingResult)
  {
    AStarAlgorithm<Vertex, Edge, Weight> algorithm;
    return ConvertTransitResult(
        mwmIds, ConvertResult<Vertex, Edge, Weight>(algorithm.FindPathBidirectional(params, routingResult)));
  }

  void SetupAlgorithmMode(IndexGraphStarter & starter, bool guidesActive = false) const;
  uint32_t ConnectTracksOnGuidesToOsm(std::vector<m2::PointD> const & checkpoints, WorldGraph & graph);

  void ConnectCheckpointsOnGuidesToOsm(std::vector<m2::PointD> const & checkpoints, WorldGraph & graph);

  void AddGuidesOsmConnectionsToGraphStarter(size_t checkpointIdxFrom, size_t checkpointIdxTo,
                                             IndexGraphStarter & starter);

  void AppendPartsOfReal(LatLonWithAltitude const & point1, LatLonWithAltitude const & point2, uint32_t & startIdx,
                         ConnectionToOsm & link);

  std::vector<Segment> GetBestOutgoingSegments(m2::PointD const & checkpoint, WorldGraph & graph);

  VehicleType m_vehicleType;
  bool m_loadAltitudes;
  std::string const m_name;
  MwmDataSource m_dataSource;
  std::shared_ptr<VehicleModelFactoryInterface> m_vehicleModelFactory;

  TCountryFileFn const m_countryFileFn;
  CountryRectFn const m_countryRectFn;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::shared_ptr<m4::Tree<NumMwmId>> m_numMwmTree;
  std::shared_ptr<TrafficStash> m_trafficStash;
  FeaturesRoadGraphBase m_roadGraph;

  std::shared_ptr<EdgeEstimator> m_estimator;
  std::unique_ptr<DirectionsEngine> m_directionsEngine;
  std::unique_ptr<SegmentedRoute> m_lastRoute;
  std::unique_ptr<FakeEdgesContainer> m_lastFakeEdges;

  // If a ckeckpoint is near to the guide track we need to build route through this track.
  GuidesConnections m_guides;

  CountryParentNameGetterFn m_countryParentNameGetterFn;
};
}  // namespace routing
