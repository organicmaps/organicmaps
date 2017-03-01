#pragma once

#include "routing_common/vehicle_model.hpp"

#include "std/shared_ptr.hpp"

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();

  static CarModel const & AllLimitsInstance();
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory();

  // VehicleModelFactory overrides:
  shared_ptr<IVehicleModel> GetVehicleModel() const override;
  shared_ptr<IVehicleModel> GetVehicleModelForCountry(string const & country) const override;

private:
  shared_ptr<IVehicleModel> m_model;
};
}  // namespace routing
