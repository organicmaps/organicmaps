#pragma once

#include "routing/directions_engine.hpp"

namespace routing
{

class PedestrianDirectionsEngine : public IDirectionsEngine
{
public:
  PedestrianDirectionsEngine();

  // IDirectionsEngine override:
  void Generate(IRoadGraph const & graph, vector<Junction> const & path, Route::TTimes & times,
                Route::TTurns & turns, vector<m2::PointD> & routeGeometry,
                my::Cancellable const & cancellable) override;

private:
  void CalculateTurns(IRoadGraph const & graph, vector<Edge> const & routeEdges,
                      Route::TTurns & turnsDir,
                      my::Cancellable const & cancellable) const;

  uint32_t const m_typeSteps;
  uint32_t const m_typeLiftGate;
  uint32_t const m_typeGate;
};

}  // namespace routing
