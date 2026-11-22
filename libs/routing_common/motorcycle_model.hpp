#pragma once

#include "routing_common/vehicle_model.hpp"

#include <string>

namespace routing
{
enum class MotorcycleProfile
{
  Default = 0,
  Fun = 1,
  Offroad = 2,
  HardEnduro = 3,
  Touring = 4
};

std::string DebugPrint(MotorcycleProfile profile);

class MotorcycleModel : public VehicleModel
{
public:
  explicit MotorcycleModel(MotorcycleProfile profile = MotorcycleProfile::Default);
  explicit MotorcycleModel(LimitsInitList const & roadLimits, MotorcycleProfile profile = MotorcycleProfile::Default);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const override;
  SpeedKMpH const & GetOffroadSpeed() const override;

  MotorcycleProfile GetProfile() const { return m_profile; }

  static MotorcycleModel const & AllLimitsInstance(MotorcycleProfile profile = MotorcycleProfile::Default);
  static LimitsInitList const & GetOptions(MotorcycleProfile profile = MotorcycleProfile::Default);
  static SurfaceInitList const & GetSurfaces(MotorcycleProfile profile = MotorcycleProfile::Default);

private:
  MotorcycleProfile m_profile;
};

class MotorcycleModelFactory : public VehicleModelFactory
{
public:
  explicit MotorcycleModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn,
                                  MotorcycleProfile profile = MotorcycleProfile::Default);
};
}  // namespace routing
