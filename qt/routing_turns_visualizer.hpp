#pragma once

#include "map/routing_manager.hpp"

#include "routing/turns.hpp"

#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/drape_engine.hpp"

#include "geometry/point2d.hpp"

#include <string>
#include <unordered_set>

namespace qt
{
class RoutingTurnsVisualizer
{
public:
  // Shows routing turns on the map with its titles.
  void Visualize(RoutingManager & routingManager, df::DrapeApi & drape);

  // Removes turns from the map.
  void ClearTurns(df::DrapeApi & drape);

private:
  // Returns turn id consisting of its index on the polyline and the maneuver name.
  static std::string GetId(routing::turns::TurnItem const & turn);

  // Turn ids for rendering on the map and erasing by drape.
  std::unordered_set<std::string> m_turnIds;
};
}  // namespace qt
