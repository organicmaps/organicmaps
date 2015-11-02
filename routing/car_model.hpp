#pragma once

#include "vehicle_model.hpp"

namespace routing
{

class CarModel : public VehicleModel
{
  CarModel();

public:
  static CarModel const & Instance();
};

}  // namespace routing
