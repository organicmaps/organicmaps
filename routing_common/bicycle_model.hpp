#pragma once
#include "routing_common/vehicle_model.hpp"

namespace routing
{

class BicycleModel : public VehicleModel
{
  explicit BicycleModel(HighwaySpeeds const & speeds);
  friend class BicycleModelFactory;

public:
  static BicycleModel const & AllLimitsInstance();

  /// @name VehicleModel overrides
  /// @{
  SpeedKMpH GetOffroadSpeed() const override;

protected:
  ResultT IsOneWay(uint32_t type) const override;
  ResultT GetRoadAvailability(uint32_t type) const override;
  SpeedKMpH GetSpeedForAvailable() const override;
  /// @}

private:
  uint32_t m_noBicycleType;
  uint32_t m_yesBicycleType;
  uint32_t m_bidirBicycleType;
  uint32_t m_onedirBicycleType;
};

class BicycleModelFactory : public VehicleModelFactory
{
public:
  explicit BicycleModelFactory(CountryParentNameGetterFn const & parentGetter);
};

}  // namespace routing
