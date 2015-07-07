#pragma once

#include "std/shared_ptr.hpp"
#include "std/unordered_map.hpp"

#include "vehicle_model.hpp"

namespace routing
{

class PedestrianModel : public VehicleModel
{
public:
  PedestrianModel();
  PedestrianModel(VehicleModel::InitListT const & speedLimits);

  /// @name Overrides from VehicleModel.
  //@{
  double GetSpeed(FeatureType const & f) const override;
  bool IsOneWay(FeatureType const &) const override { return false; }
  //@}

private:
  void Init();
  bool IsFoot(feature::TypesHolder const & types) const;

  uint32_t m_noFootType;
};

class PedestrianModelFactory : public IVehicleModelFactory
{
public:
  PedestrianModelFactory();

  /// @name Overrides from IVehicleModelFactory.
  //@{
  shared_ptr<IVehicleModel> GetVehicleModel() const override;
  shared_ptr<IVehicleModel> GetVehicleModelForCountry(string const & country) const override;
  //@}

private:
  unordered_map<string, shared_ptr<IVehicleModel>> m_models;
};

}  // namespace routing
