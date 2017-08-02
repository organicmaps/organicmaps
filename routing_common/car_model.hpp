#pragma once

#include "routing_common/vehicle_model.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();
  CarModel(VehicleModel::InitListT const & roadLimits);

  static CarModel const & AllLimitsInstance();

private:
  void InitAdditionalRoadTypes();
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory();

  // VehicleModelFactory overrides:
  std::shared_ptr<IVehicleModel> GetVehicleModel() const override;
  std::shared_ptr<IVehicleModel> GetVehicleModelForCountry(std::string const & country) const override;

private:
  std::unordered_map<std::string, std::shared_ptr<IVehicleModel>> m_models;
};
}  // namespace routing
