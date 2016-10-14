#pragma once

#include "routing/bicycle_model.hpp"
#include "routing/car_model.hpp"
#include "routing/pedestrian_model.hpp"

namespace routing
{
/// \returns true when there exists a routing mode where the feature with |types| can be used.
template <class TTypes>
bool IsRoad(TTypes const & types)
{
  return CarModel::AllLimitsInstance().HasRoadType(types) ||
         PedestrianModel::AllLimitsInstance().HasRoadType(types) ||
         BicycleModel::AllLimitsInstance().HasRoadType(types);
}
}  // namespace rouing
