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

  /// VehicleModel overrides:
  double GetSpeed(FeatureType const & f) const override;
  bool IsOneWay(FeatureType const &) const override { return false; }
  /// @returns true if |f| could be considered as a road for pedestrian routing.
  /// @note If PedestrianModel::IsRoad(f) returns false for a feature f and for an instance
  /// of |PedestrianModel| created by default constructor
  /// PedestrianModel::IsRoad(f) for the same feature f and for any instance
  /// of |PedestrianModel| created by |PedestrianModel(VehicleModel::InitListT const &)| must return false.
  bool IsRoad(FeatureType const & f) const override;

private:
  void Init();

  /// @return Restriction::Yes if road is prohibited for pedestrian.
  Restriction IsNoFoot(feature::TypesHolder const & types) const;

  /// @return Restriction::Yes if road is allowed for pedestrian.
  Restriction IsYesFoot(feature::TypesHolder const & types) const;

  uint32_t m_noFootType = 0;
  uint32_t m_yesFootType = 0;
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
