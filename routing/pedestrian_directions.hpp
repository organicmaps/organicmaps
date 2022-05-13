#pragma once

#include "routing/directions_engine.hpp"
#include "routing/directions_engine_helpers.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turn_candidate.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point_with_altitude.hpp"

#include <memory>
#include <vector>

namespace routing
{
class PedestrianDirectionsEngine : public DirectionsEngine
{
public:
  PedestrianDirectionsEngine(DataSource const & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);

protected:
  virtual size_t GetTurnDirection(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                  NumMwmIds const & numMwmIds,
                                  RoutingSettings const & vehicleSettings, TurnItem & turn);
  virtual void FixupTurns(std::vector<geometry::PointWithAltitude> const & junctions,
                          Route::TTurns & turnsDir);
};
}  // namespace routing
