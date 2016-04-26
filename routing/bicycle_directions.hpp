#pragma once

#include "routing/directions_engine.hpp"
#include "routing/turn_candidate.hpp"

#include "std/map.hpp"

namespace routing
{
struct AdjacentEdges
{
  turns::TTurnCandidates outgoingTurns;
  size_t ingoingTurnCount;
};

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
  map<TNodeId, AdjacentEdges> m_adjacentEdges;
};
}  // namespace routing
