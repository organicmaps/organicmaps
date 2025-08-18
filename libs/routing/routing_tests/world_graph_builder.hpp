#pragma once

#include "routing/single_vehicle_world_graph.hpp"

#include <memory>

namespace routing_test
{
std::unique_ptr<routing::SingleVehicleWorldGraph> BuildCrossGraph();
std::unique_ptr<routing::SingleVehicleWorldGraph> BuildTestGraph();
}  // namespace routing_test
