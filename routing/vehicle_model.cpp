#include "routing/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/macros.hpp"

#include "std/limits.hpp"
#include "std/initializer_list.hpp"


namespace routing
{

initializer_list<VehicleModel::SpeedForType> const s_carLimits = {
  { {"highway", "motorway"},       90 },
  { {"highway", "trunk"},          85 },
  { {"highway", "motorway_link"},  75 },
  { {"highway", "trunk_link"},     70 },
  { {"highway", "primary"},        65 },
  { {"highway", "primary_link"},   60 },
  { {"highway", "secondary"},      55 },
  { {"highway", "secondary_link"}, 50 },
  { {"highway", "tertiary"},       40 },
  { {"highway", "tertiary_link"},  30 },
  { {"highway", "residential"},    25 },
  { {"highway", "pedestrian"},     25 },
  { {"highway", "unclassified"},   25 },
  { {"highway", "service"},        15 },
  { {"highway", "living_street"},  10 },
  { {"highway", "road"},           10 },
  { {"highway", "track"},          5  },
  /// @todo: Add to classificator
  //{ {"highway", "shuttle_train"},  10 },
  //{ {"highway", "ferry"},          5  },
  //{ {"highway", "default"},        10 },
  /// @todo: check type
  //{ {"highway", "construction"},   40 },
};


CarModel::CarModel()
  : VehicleModel(classif(), s_carLimits)
{
}

VehicleModel::VehicleModel(Classificator const & c, vector<SpeedForType> const & speedLimits)
  : m_maxSpeed(0),
    m_onewayType(c.GetTypeByPath({ "hwtag", "oneway" }))
{
  for (size_t i = 0; i < speedLimits.size(); ++i)
  {
    m_maxSpeed = max(m_maxSpeed, speedLimits[i].m_speed);
    m_types[c.GetTypeByPath(vector<string>(speedLimits[i].m_types, speedLimits[i].m_types + 2))] = speedLimits[i];
  }

  initializer_list<char const *> arr[] = {
    { "route", "ferry", "motorcar" },
    { "route", "ferry", "motor_vehicle" },
    { "railway", "rail", "motor_vehicle" },
  };
  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_addRoadTypes.push_back(c.GetTypeByPath(arr[i]));
}

double VehicleModel::GetSpeed(FeatureType const & f) const
{
  return GetSpeed(feature::TypesHolder(f));
}

double VehicleModel::GetSpeed(feature::TypesHolder const & types) const
{
  double speed = m_maxSpeed * 2;
  for (uint32_t t : types)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareToMatch(t, 2);
    auto it = m_types.find(type);
    if (it != m_types.end())
      speed = min(speed, it->second.m_speed);
  }
  if (speed <= m_maxSpeed)
    return speed;

  return 0.0;
}

bool VehicleModel::IsOneWay(FeatureType const & f) const
{
  return IsOneWay(feature::TypesHolder(f));
}

bool VehicleModel::IsOneWay(feature::TypesHolder const & types) const
{
  return types.Has(m_onewayType);
}

bool VehicleModel::IsRoad(FeatureType const & f) const
{
  feature::TypesHolder types(f);
  for (uint32_t t : types)
    if (IsRoad(t))
      return true;

  return false;
}

bool VehicleModel::IsRoad(vector<uint32_t> const & types) const
{
  for (uint32_t t : types)
    if (IsRoad(t))
      return true;

  return false;
}

bool VehicleModel::IsRoad(uint32_t type) const
{
  return (find(m_addRoadTypes.begin(), m_addRoadTypes.end(), type) != m_addRoadTypes.end() ||
          m_types.find(ftypes::BaseChecker::PrepareToMatch(type, 2)) != m_types.end());
}

}
