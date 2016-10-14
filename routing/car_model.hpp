#pragma once
#include "vehicle_model.hpp"

#include "std/shared_ptr.hpp"

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();

  static CarModel const & AllLimitsInstance();
};

class CarModelFactory : public IVehicleModelFactory
{
public:
  CarModelFactory();

  // IVehicleModelFactory overrides:
  shared_ptr<IVehicleModel> GetVehicleModel() const override;
  shared_ptr<IVehicleModel> GetVehicleModelForCountry(string const & country) const override;

private:
  shared_ptr<IVehicleModel> m_model;
};
}  // namespace routing
