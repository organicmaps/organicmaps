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
  SpeedKMpH const & GetOffroadSpeed() const override;

  static CarModel const & AllLimitsInstance();
  static LimitsInitList const & GetOptions();
  static std::vector<AdditionalRoadTags> const & GetAdditionalTags();
  static VehicleModel::SurfaceInitList const & GetSurfaces();

  uint32_t GetNoCarTypeForTesting() const { return m_noCarType; }
  uint32_t GetYesCarTypeForTesting() const { return m_yesCarType; }

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
