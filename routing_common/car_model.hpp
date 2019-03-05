#pragma once

#include "routing_common/vehicle_model.hpp"

#include <vector>

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();
  CarModel(VehicleModel::LimitsInitList const & roadLimits, HighwayBasedInfo const & info);

  // VehicleModelInterface overrides:
  double GetOffroadSpeed() const override;

  static CarModel const & AllLimitsInstance();
  static LimitsInitList const & GetOptions();
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
