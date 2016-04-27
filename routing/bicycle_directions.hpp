#pragma once

#include "routing/directions_engine.hpp"
#include "loaded_path_segment.hpp"
#include "routing/turn_candidate.hpp"

#include "std/map.hpp"

namespace routing
{
struct AdjacentEdges
{
  turns::TTurnCandidates outgoingTurns;
  size_t ingoingTurnCount;
};

using AdjacentEdgesMap = map<TNodeId, AdjacentEdges>;

class BicycleDirectionsEngine : public IDirectionsEngine
{
public:
  BicycleDirectionsEngine();

  // IDirectionsEngine override:
  void Generate(IRoadGraph const & graph, vector<Junction> const & path,
                Route::TTimes & times,
                Route::TTurns & turnsDir,
                my::Cancellable const & cancellable) override;
private:
  AdjacentEdgesMap m_adjacentEdges;
  TUnpackedPathSegments m_pathSegments;
};
}  // namespace routing
