#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class PedestrianModel : public VehicleModel
{
  explicit PedestrianModel(HighwaySpeeds const & speeds);
  friend class PedestrianModelFactory;

public:
  static PedestrianModel const & AllLimitsInstance();

  /// @name VehicleModelInterface overrides
  /// @{
  SpeedKMpH GetOffroadSpeed() const override;

protected:
  ResultT IsOneWay(uint32_t type) const override;
  ResultT GetRoadAvailability(uint32_t type) const override;
  SpeedKMpH GetSpeedForAvailable() const override;
  /// @}

private:
  uint32_t m_noFootType = 0;
  uint32_t m_yesFootType = 0;
};

class PedestrianModelFactory : public VehicleModelFactory
{
public:
  explicit PedestrianModelFactory(CountryParentNameGetterFn const & parentGetter);
};

}  // namespace routing
