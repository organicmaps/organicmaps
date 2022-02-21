#pragma once

#include "routing_common/vehicle_model.hpp"

#include <vector>

namespace routing
{

class CarModel : public VehicleModel
{
public:
  CarModel();
  CarModel(LimitsInitList const & roadLimits, HighwayBasedInfo const & info);

  // VehicleModelInterface overrides:
  SpeedKMpH const & GetOffroadSpeed() const override;

  static CarModel const & AllLimitsInstance();
  static LimitsInitList const & GetOptions();
  static AdditionalRoadsList const & GetAdditionalRoads();
  static SurfaceInitList const & GetSurfaces();

protected:
  RoadAvailability GetRoadAvailability(feature::TypesHolder const & types) const override;

private:
  void Init();

  uint32_t m_noCarType = 0;
  uint32_t m_yesCarType = 0;
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
