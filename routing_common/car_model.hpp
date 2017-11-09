#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();
  CarModel(VehicleModel::InitListT const & roadLimits);

  // VehicleModelInterface overrides
  double GetOffroadSpeed() const override;

  static CarModel const & AllLimitsInstance();
  static InitListT const & GetLimits();
  static std::vector<AdditionalRoadTags> const & GetAdditionalTags();

private:
  void InitAdditionalRoadTypes();
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
