#pragma once

#include "routing/directions_engine.hpp"

namespace routing
{

class PedestrianDirectionsEngine : public IDirectionsEngine
{
public:
  PedestrianDirectionsEngine();

  // IDirectionsEngine override:
  void Generate(IRoadGraph const & graph, vector<Junction> const & path,
                Route::TTimes & times,
                Route::TTurns & turnsDir,
                turns::TTurnsGeom & turnsGeom) override;

private:
  bool ReconstructPath(IRoadGraph const & graph, vector<Junction> const & path,
                       vector<Edge> & routeEdges) const;

  void CalculateTimes(IRoadGraph const & graph, vector<Junction> const & path,
                      Route::TTimes & times) const;

  void CalculateTurns(IRoadGraph const & graph, vector<Edge> const & routeEdges,
                      Route::TTurns & turnsDir) const;

  uint32_t const m_typeSteps;
  uint32_t const m_typeLiftGate;
};

}  // namespace routing
