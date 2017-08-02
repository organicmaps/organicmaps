#pragma once

#include "routing_common/vehicle_model.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace routing
{

class PedestrianModel : public VehicleModel
{
public:
  PedestrianModel();
  PedestrianModel(VehicleModel::InitListT const & speedLimits);

  /// VehicleModel overrides:
  bool IsOneWay(FeatureType const &) const override { return false; }
  static PedestrianModel const & AllLimitsInstance();

protected:
  RoadAvailability GetRoadAvailability(feature::TypesHolder const & types) const override;

private:
  void Init();

  uint32_t m_noFootType = 0;
  uint32_t m_yesFootType = 0;
};

class PedestrianModelFactory : public VehicleModelFactory
{
public:
  PedestrianModelFactory();

  /// @name Overrides from VehicleModelFactory.
  //@{
  std::shared_ptr<IVehicleModel> GetVehicleModel() const override;
  std::shared_ptr<IVehicleModel> GetVehicleModelForCountry(std::string const & country) const override;
  //@}

private:
  std::unordered_map<std::string, std::shared_ptr<IVehicleModel>> m_models;
};
}  // namespace routing
