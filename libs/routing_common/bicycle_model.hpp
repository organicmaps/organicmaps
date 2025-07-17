#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class BicycleModel : public VehicleModel
{
public:
  BicycleModel();
  explicit BicycleModel(VehicleModel::LimitsInitList const & limits);
  BicycleModel(VehicleModel::LimitsInitList const & limits, HighwayBasedSpeeds const & speeds);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const override;
  bool IsOneWay(FeatureTypes const & types) const override;
  SpeedKMpH const & GetOffroadSpeed() const override;

  static BicycleModel const & AllLimitsInstance();
  static SpeedKMpH DismountSpeed();

private:
  /// @return true if it is allowed to ride a bicycle in both directions.
  bool IsBicycleBidir(feature::TypesHolder const & types) const;
  // Returns true if the road is explicitly set oneway for bicycles.
  bool IsBicycleOnedir(feature::TypesHolder const & types) const;

  uint32_t m_bidirBicycleType = 0;
  uint32_t m_onedirBicycleType = 0;
};

class BicycleModelFactory : public VehicleModelFactory
{
public:
  // TODO: remove countryParentNameGetterFn default value after removing unused bicycle routing
  // from road_graph_router
  BicycleModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn = {});
};
}  // namespace routing
