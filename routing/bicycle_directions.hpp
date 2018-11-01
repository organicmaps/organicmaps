#pragma once

#include "routing/directions_engine.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/segment.hpp"
#include "routing/turn_candidate.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "indexer/data_source.hpp"

#include <map>
#include <memory>

namespace routing
{
class BicycleDirectionsEngine : public IDirectionsEngine
{
public:
  struct AdjacentEdges
  {
    explicit AdjacentEdges(size_t ingoingTurnsCount = 0) : m_ingoingTurnsCount(ingoingTurnsCount) {}
    bool IsAlmostEqual(AdjacentEdges const & rhs) const;

    turns::TurnCandidates m_outgoingTurns;
    size_t m_ingoingTurnsCount;
  };

  using AdjacentEdgesMap = std::map<SegmentRange, AdjacentEdges>;

  BicycleDirectionsEngine(DataSource const & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);

  // IDirectionsEngine override:
  bool Generate(IndexRoadGraph const & graph, vector<Junction> const & path,
                base::Cancellable const & cancellable, Route::TTurns & turns,
                Route::TStreets & streetNames, vector<Junction> & routeGeometry,
                vector<Segment> & segments) override;

private:
  FeaturesLoaderGuard & GetLoader(MwmSet::MwmId const & id);
  void LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment);
  void GetSegmentRangeAndAdjacentEdges(IRoadGraph::TEdgeVector const & outgoingEdges,
                                       Edge const & inEdge, uint32_t startSegId, uint32_t endSegId,
                                       SegmentRange & segmentRange,
                                       turns::TurnCandidates & outgoingTurns);
  /// \brief The method gathers sequence of segments according to IsJoint() method
  /// and fills |m_adjacentEdges| and |m_pathSegments|.
  void FillPathSegmentsAndAdjacentEdgesMap(IndexRoadGraph const & graph,
                                           std::vector<Junction> const & path,
                                           IRoadGraph::TEdgeVector const & routeEdges,
                                           base::Cancellable const & cancellable);

  void GetEdges(RoadGraphBase const & graph, Junction const & currJunction,
                bool isCurrJunctionFinish, IRoadGraph::TEdgeVector & outgoing,
                IRoadGraph::TEdgeVector & ingoing);

  AdjacentEdgesMap m_adjacentEdges;
  TUnpackedPathSegments m_pathSegments;
  DataSource const & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::unique_ptr<FeaturesLoaderGuard> m_loader;
};
}  // namespace routing
