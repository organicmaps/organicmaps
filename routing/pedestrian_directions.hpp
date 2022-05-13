#pragma once

#include "routing/directions_engine.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>
#include <vector>

namespace routing
{
class PedestrianDirectionsEngine : public DirectionsEngine
{
public:
  PedestrianDirectionsEngine(DataSource const & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);

protected:
  virtual size_t GetTurnDirection(turns::IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                  NumMwmIds const & numMwmIds,
                                  RoutingSettings const & vehicleSettings, turns::TurnItem & turn);
  virtual void FixupTurns(std::vector<geometry::PointWithAltitude> const & junctions,
                          Route::TTurns & turnsDir);
};
}  // namespace routing
