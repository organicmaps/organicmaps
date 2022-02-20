#pragma once

#include "routing_common/vehicle_model.hpp"

#include <vector>

namespace routing
{

class CarModel : public VehicleModel
{
  explicit CarModel(HighwaySpeeds const & speeds, NoPassThroughHighways const & noPassThrough = {});
  friend class CarModelFactory;

public:
  static CarModel const & AllLimitsInstance();

  /// @name VehicleModelInterface overrides
  /// @{
  SpeedKMpH GetOffroadSpeed() const override;

protected:
  ResultT IsOneWay(uint32_t type) const override;
  ResultT GetRoadAvailability(uint32_t type) const override;
  SpeedKMpH GetSpeedForAvailable() const override;
  /// @}

private:
  uint32_t m_noCarType = 0;
  uint32_t m_yesCarType = 0;
  uint32_t m_onewayType = 0;
};

class CarModelFactory : public VehicleModelFactory
{
public:
  explicit CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
