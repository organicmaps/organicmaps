#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class BicycleModel : public VehicleModel
{
public:
  BicycleModel();
  BicycleModel(VehicleModel::LimitsInitList const & speedLimits);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureType & f, SpeedParams const & speedParams) const override;
  bool IsOneWay(FeatureType & f) const override;
  double GetOffroadSpeed() const override;

  static BicycleModel const & AllLimitsInstance();

protected:
  RoadAvailability GetRoadAvailability(feature::TypesHolder const & types) const override;

private:
  void Init();

  /// @return true if it is allowed to ride a bicycle in both directions.
  bool IsBicycleBidir(feature::TypesHolder const & types) const;

  uint32_t m_noBicycleType = 0;
  uint32_t m_yesBicycleType = 0;
  uint32_t m_bidirBicycleType = 0;
};

class BicycleModelFactory : public VehicleModelFactory
{
public:
  // TODO: remove countryParentNameGetterFn default value after removing unused bicycle routing
  // from road_graph_router
  BicycleModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn = {});
};
}  // namespace routing
