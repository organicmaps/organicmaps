#pragma once

#include "routing/directions_engine_helpers.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/road_point.hpp"
#include "routing/route.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "traffic/traffic_info.hpp"

#include "indexer/data_source.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"

#include <memory>
#include <vector>

namespace routing
{
class DirectionsEngine
{
public:
  DirectionsEngine(DataSource const & dataSource, std::shared_ptr<NumMwmIds> numMwmIds)
    : m_dataSource(dataSource), m_numMwmIds(numMwmIds)
  {
    CHECK(m_numMwmIds, ());
  }

  virtual ~DirectionsEngine() = default;

  // @TODO(bykoianko) Method Generate() should fill
  // vector<RouteSegment> instead of corresponding arguments.
  /// \brief Generates all args which are passed by reference.
  /// \param path is points of the route. It should not be empty.
  /// \returns true if fields passed by reference are filled correctly and false otherwise.
  virtual bool Generate(IndexRoadGraph const & graph,
                        std::vector<geometry::PointWithAltitude> const & path,
                        base::Cancellable const & cancellable, Route::TTurns & turns,
                        Route::TStreets & streetNames,
                        std::vector<geometry::PointWithAltitude> & routeGeometry,
                        std::vector<Segment> & segments) = 0;
  void Clear();

  void SetVehicleType(VehicleType const & vehicleType) { m_vehicleType = vehicleType; }

protected:
  FeaturesLoaderGuard & GetLoader(MwmSet::MwmId const & id);
  void LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment);
  void GetSegmentRangeAndAdjacentEdges(IRoadGraph::EdgeVector const & outgoingEdges,
                                       Edge const & inEdge, uint32_t startSegId, uint32_t endSegId,
                                       SegmentRange & segmentRange,
                                       turns::TurnCandidates & outgoingTurns);
  /// \brief The method gathers sequence of segments according to IsJoint() method
  /// and fills |m_adjacentEdges| and |m_pathSegments|.
  void FillPathSegmentsAndAdjacentEdgesMap(IndexRoadGraph const & graph,
                                           std::vector<geometry::PointWithAltitude> const & path,
                                           IRoadGraph::EdgeVector const & routeEdges,
                                           base::Cancellable const & cancellable);

  void GetEdges(IndexRoadGraph const & graph, geometry::PointWithAltitude const & currJunction,
                bool isCurrJunctionFinish, IRoadGraph::EdgeVector & outgoing,
                IRoadGraph::EdgeVector & ingoing);

  AdjacentEdgesMap m_adjacentEdges;
  TUnpackedPathSegments m_pathSegments;

  DataSource const & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::unique_ptr<FeaturesLoaderGuard> m_loader;
  VehicleType m_vehicleType = VehicleType::Count;
};
}  // namespace routing
