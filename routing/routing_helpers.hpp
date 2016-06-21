#pragma once

#include "routing/bicycle_model.hpp"
#include "routing/car_model.hpp"
#include "routing/pedestrian_model.hpp"

namespace routing
{
/// \returns true if a feature with |types| can be used for any kind of routing.
template <class TList>
bool IsRoad(TList const & types)
{
  return CarModel::Instance().HasRoadType(types)
         || PedestrianModel::DefaultInstance().HasRoadType(types)
         || BicycleModel::DefaultInstance().HasRoadType(types);
}
} // namespace rouing
