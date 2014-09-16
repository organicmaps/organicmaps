#include "vehicle_model.hpp"
#include "../indexer/classificator.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"
#include "../base/macros.hpp"
#include "../std/limits.hpp"

namespace routing
{

VehicleModel::SpeedForType const s_carLimits[] = {
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
  : VehicleModel(classif(), vector<VehicleModel::SpeedForType>(s_carLimits, s_carLimits + ARRAY_SIZE(s_carLimits)))
{
}

VehicleModel::VehicleModel(Classificator const & c, vector<SpeedForType> const & speedLimits) : m_maxSpeed(0)
{
  char const * arr[] = { "hwtag", "oneway" };
  m_onewayType = c.GetTypeByPath(vector<string>(arr, arr + 2));

  for (size_t i = 0; i < speedLimits.size(); ++i)
  {
    m_maxSpeed = max(m_maxSpeed, speedLimits[i].m_speed);
    m_types[c.GetTypeByPath(vector<string>(speedLimits[i].m_types, speedLimits[i].m_types + 2))] = speedLimits[i];
  }
}

double VehicleModel::GetSpeed(FeatureType const & f) const
{
  return GetSpeed(feature::TypesHolder(f));
}

double VehicleModel::GetSpeed(feature::TypesHolder const & types) const
{
  double speed = m_maxSpeed * 2;
  for (size_t i = 0; i < types.Size(); ++i)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareFeatureTypeToMatch(types[i]);
    TypesT::const_iterator it = m_types.find(type);
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

}
