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
  /// @returns true if |f| could be considered as a road.
  /// @note If BicycleModel::IsRoad(f) returns false for a feature f and for an instance
  /// of |BicycleModel| created by default constructor
  /// BicycleModel::IsRoad(f) for the same feature f and for any instance
  /// of |BicycleModel| created by |BicycleModel(VehicleModel::InitListT const &)| must return false.
  bool IsRoad(FeatureType const & f) const override;

private:
  void Init();

  /// @return Restriction::Yes if road is prohibited for bicycle.
  Restriction IsNoBicycle(feature::TypesHolder const & types) const;

  /// @return Restriction::Yes if road is allowed for bicycle.
  Restriction IsYesBicycle(feature::TypesHolder const & types) const;

  /// @return Restriction::Yes if it is allowed to ride bicycle in two directions.
  Restriction IsBicycleBidir(feature::TypesHolder const & types) const;

  uint32_t m_noBicycleType = 0;
  uint32_t m_yesBicycleType = 0;
  uint32_t m_bicycleBidirType = 0;
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
