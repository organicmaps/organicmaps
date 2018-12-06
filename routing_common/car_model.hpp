#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();
  CarModel(VehicleModel::LimitsInitList const & roadLimits);

  // VehicleModelInterface overrides:
  double GetOffroadSpeed() const override;

  static CarModel const & AllLimitsInstance();
  static LimitsInitList const & GetLimits();
  static std::vector<AdditionalRoadTags> const & GetAdditionalTags();
  static VehicleModel::SurfaceInitList const & GetSurfaces();

private:
  void InitAdditionalRoadTypes();
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
