#pragma once

#include "routing/directions_engine_helpers.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/route.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/feature.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/cancellable.hpp"

#include <memory>
#include <vector>

namespace routing
{
namespace turns
{
class IRoutingResult;
struct TurnItem;
}  // namespace turns

enum class RouterResultCode;
class MwmDataSource;

class DirectionsEngine
{
public:
  DirectionsEngine(MwmDataSource & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);
  virtual ~DirectionsEngine() = default;

  // @TODO(bykoianko) Method Generate() should fill
  // vector<RouteSegment> instead of corresponding arguments.
  /// \brief Generates all args which are passed by reference.
  /// \param path is points of the route. It should not be empty.
  /// \returns true if fields passed by reference are filled correctly and false otherwise.
  bool Generate(IndexRoadGraph const & graph, std::vector<geometry::PointWithAltitude> const & path,
                base::Cancellable const & cancellable, std::vector<RouteSegment> & routeSegments);
  void Clear();

  void SetVehicleType(VehicleType const & vehicleType) { m_vehicleType = vehicleType; }

protected:
  /*!
   * \brief GetTurnDirection makes a primary decision about turns on the route.
   * \param outgoingSegmentIndex index of an outgoing segments in vector result.GetSegments().
   * \param turn is used for keeping the result of turn calculation.
   */
  virtual size_t GetTurnDirection(turns::IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                  NumMwmIds const & numMwmIds, RoutingSettings const & vehicleSettings,
                                  turns::TurnItem & turn) = 0;
  virtual void FixupTurns(std::vector<RouteSegment> & routeSegments) = 0;
  std::unique_ptr<FeatureType> GetFeature(FeatureID const & featureId);
  void LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment, bool isForward);
  void GetSegmentRangeAndAdjacentEdges(IRoadGraph::EdgeListT const & outgoingEdges, Edge const & inEdge,
                                       uint32_t startSegId, uint32_t endSegId, SegmentRange & segmentRange,
                                       turns::TurnCandidates & outgoingTurns);
  /// \brief The method gathers sequence of segments according to IsJoint() method
  /// and fills |m_adjacentEdges| and |m_pathSegments|.
  void FillPathSegmentsAndAdjacentEdgesMap(IndexRoadGraph const & graph,
                                           std::vector<geometry::PointWithAltitude> const & path,
                                           IRoadGraph::EdgeVector const & routeEdges,
                                           base::Cancellable const & cancellable);

  AdjacentEdgesMap m_adjacentEdges;
  TUnpackedPathSegments m_pathSegments;

  MwmDataSource & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  VehicleType m_vehicleType = VehicleType::Count;

private:
  void MakeTurnAnnotation(IndexRoadGraph::EdgeVector const & routeEdges, std::vector<RouteSegment> & routeSegments);

  ftypes::IsLinkChecker const & m_linkChecker;
  ftypes::IsRoundAboutChecker const & m_roundAboutChecker;
  ftypes::IsOneWayChecker const & m_onewayChecker;
};
}  // namespace routing
