#pragma once

#include "routing/directions_engine.hpp"

namespace routing
{

class PedestrianDirectionsEngine : public IDirectionsEngine
{
public:
  PedestrianDirectionsEngine();

  // IDirectionsEngine override:
  void Generate(RoadGraphBase const & graph, vector<Junction> const & path,
                my::Cancellable const & cancellable, Route::TTimes & times, Route::TTurns & turns,
                vector<Junction> & routeGeometry, vector<Segment> & /* trafficSegs */) override;

private:
  void CalculateTurns(RoadGraphBase const & graph, vector<Edge> const & routeEdges,
                      Route::TTurns & turnsDir, my::Cancellable const & cancellable) const;

  uint32_t const m_typeSteps;
  uint32_t const m_typeLiftGate;
  uint32_t const m_typeGate;
};

}  // namespace routing
