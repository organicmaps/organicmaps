#pragma once

#include "routing/directions_engine.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>

namespace routing
{

class PedestrianDirectionsEngine : public IDirectionsEngine
{
public:
  PedestrianDirectionsEngine(std::shared_ptr<NumMwmIds> numMwmIds);

  // IDirectionsEngine override:
  bool Generate(IndexRoadGraph const & graph, vector<Junction> const & path,
                base::Cancellable const & cancellable, Route::TTurns & turns,
                Route::TStreets & streetNames, vector<Junction> & routeGeometry,
                vector<Segment> & segments) override;

private:
  void CalculateTurns(IndexRoadGraph const & graph, std::vector<Edge> const & routeEdges,
                      Route::TTurns & turnsDir, base::Cancellable const & cancellable) const;

  uint32_t const m_typeSteps;
  uint32_t const m_typeLiftGate;
  uint32_t const m_typeGate;
  std::shared_ptr<NumMwmIds> const m_numMwmIds;
};

}  // namespace routing
