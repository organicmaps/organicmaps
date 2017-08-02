#pragma once

#include "routing_common/vehicle_model.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace routing
{

class BicycleModel : public VehicleModel
{
public:
  BicycleModel();
  BicycleModel(VehicleModel::InitListT const & speedLimits);

  /// VehicleModel overrides:
  bool IsOneWay(FeatureType const & f) const override;

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
  BicycleModelFactory();

  /// @name Overrides from VehicleModelFactory.
  //@{
  std::shared_ptr<IVehicleModel> GetVehicleModel() const override;
  std::shared_ptr<IVehicleModel> GetVehicleModelForCountry(std::string const & country) const override;
  //@}

private:
  std::unordered_map<std::string, std::shared_ptr<IVehicleModel>> m_models;
};
}  // namespace routing
