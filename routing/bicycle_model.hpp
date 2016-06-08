#pragma once

#include "std/shared_ptr.hpp"
#include "std/unordered_map.hpp"

#include "vehicle_model.hpp"

namespace routing
{

class BicycleModel : public VehicleModel
{
public:
  BicycleModel();
  BicycleModel(VehicleModel::InitListT const & speedLimits);

  /// VehicleModel overrides:
  double GetSpeed(FeatureType const & f) const override;
  bool IsOneWay(FeatureType const & f) const override;
  bool IsRoad(FeatureType const & f) const override;

private:
  void Init();
  Restriction IsBicycleAllowed(feature::TypesHolder const & types) const;

  /// @return Restriction::Yes if it is allowed to ride bicycle in two directions.
  Restriction IsBicycleBidir(feature::TypesHolder const & types) const;

  uint32_t m_noBicycleType = 0;
  uint32_t m_yesBicycleType = 0;
  uint32_t m_bidirBicycleType = 0;
};

class BicycleModelFactory : public IVehicleModelFactory
{
public:
  BicycleModelFactory();

  /// @name Overrides from IVehicleModelFactory.
  //@{
  shared_ptr<IVehicleModel> GetVehicleModel() const override;
  shared_ptr<IVehicleModel> GetVehicleModelForCountry(string const & country) const override;
  //@}

private:
  unordered_map<string, shared_ptr<IVehicleModel>> m_models;
};

}  // namespace routing
