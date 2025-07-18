#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();
  explicit CarModel(LimitsInitList const & roadLimits);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const override;
  SpeedKMpH const & GetOffroadSpeed() const override;

  static CarModel const & AllLimitsInstance();
  static LimitsInitList const & GetOptions();
  static SurfaceInitList const & GetSurfaces();
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
