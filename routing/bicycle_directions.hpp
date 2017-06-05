#pragma once

#include "routing/directions_engine.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/turn_candidate.hpp"

#include <map>
#include <memory>

class Index;

namespace routing
{
class BicycleDirectionsEngine : public IDirectionsEngine
{
public:
  struct AdjacentEdges
  {
    explicit AdjacentEdges(size_t ingoingTurnsCount = 0) : m_ingoingTurnsCount(ingoingTurnsCount) {}

    turns::TurnCandidates m_outgoingTurns;
    size_t m_ingoingTurnsCount;
  };

  using AdjacentEdgesMap = std::map<UniNodeId, AdjacentEdges>;

  BicycleDirectionsEngine(Index const & index, std::shared_ptr<NumMwmIds> numMwmIds);

  // IDirectionsEngine override:
  void Generate(RoadGraphBase const & graph, vector<Junction> const & path,
                my::Cancellable const & cancellable, Route::TTimes & times, Route::TTurns & turns,
                vector<Junction> & routeGeometry, vector<Segment> & trafficSegs) override;

private:
  Index::FeaturesLoaderGuard & GetLoader(MwmSet::MwmId const & id);
  void LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment);
  void GetUniNodeIdAndAdjacentEdges(IRoadGraph::TEdgeVector const & outgoingEdges,
                                    FeatureID const & inFeatureId, uint32_t startSegId,
                                    uint32_t endSegId, bool inIsForward, UniNodeId & uniNodeId,
                                    BicycleDirectionsEngine::AdjacentEdges & adjacentEdges);
  /// \brief The method gathers sequence of segments according to IsJoint() method
  /// and fills |m_adjacentEdges| and |m_pathSegments|.
  void FillPathSegmentsAndAdjacentEdgesMap(RoadGraphBase const & graph,
                                           vector<Junction> const & path,
                                           IRoadGraph::TEdgeVector const & routeEdges,
                                           my::Cancellable const & cancellable);
  /// \brief This method should be called for an internal junction of the route with corresponding
  /// |ingoingEdges|, |outgoingEdges|, |ingoingRouteEdge| and |outgoingRouteEdge|.
  /// \returns false if the junction is an internal point of feature segment and can be considered as
  /// a part of LoadedPathSegment and returns true if the junction should be considered as a beginning
  /// of a new LoadedPathSegment.
  bool IsJoint(IRoadGraph::TEdgeVector const & ingoingEdges,
               IRoadGraph::TEdgeVector const & outgoingEdges, Edge const & ingoingRouteEdge,
               Edge const & outgoingRouteEdge);

  bool GetSegment(FeatureID const & featureId, uint32_t segId, bool isFeatureForward,
                  Segment & segment);

  AdjacentEdgesMap m_adjacentEdges;
  TUnpackedPathSegments m_pathSegments;
  Index const & m_index;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::unique_ptr<Index::FeaturesLoaderGuard> m_loader;
};
}  // namespace routing
