#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <sstream>

namespace routing
{
using namespace std;

template <double const & (*F)(double const &, double const &), typename WeightAndETA>
WeightAndETA Pick(WeightAndETA const & lhs, WeightAndETA const & rhs)
{
  return {F(lhs.m_weight, rhs.m_weight), F(lhs.m_eta, rhs.m_eta)};
}

InOutCitySpeedKMpH Max(InOutCitySpeedKMpH const & lhs, InOutCitySpeedKMpH const & rhs)
{
  return {Pick<max>(lhs.m_inCity, rhs.m_inCity), Pick<max>(lhs.m_outCity, rhs.m_outCity)};
}


VehicleModel::VehicleModel(HighwaySpeeds const & speeds, HighwayFactors const & factors,
                           SurfaceFactors const & surfaces, NoPassThroughHighways const & noPassThrough)
{
  m_surfaceFactors.Reserve(surfaces.size());
  for (auto const & e : surfaces)
  {
    ASSERT(e.second.IsValid(), ());
    m_surfaceFactors.Insert(static_cast<uint32_t>(e.first), e.second);
  }

  m_highwayInfo.Reserve(speeds.size());
  for (auto const & e : speeds)
  {
    ASSERT(e.second.IsValid(), ());

    Info info;
    info.m_speed = e.second;

    auto const itFactor = factors.find(e.first);
    if (itFactor != factors.end())
      info.m_factor = itFactor->second;

    if (noPassThrough.count(e.first) > 0)
      info.m_isPassThroughAllowed = false;

    m_highwayInfo.Insert(static_cast<uint32_t>(e.first), info);

    m_maxModelSpeed = Max(m_maxModelSpeed, info.m_speed);
  }
  m_highwayInfo.FinishBuilding();
}

VehicleModel::Response VehicleModel::GetFeatureInfo(FeatureType & ft, Request const & request) const
{
  return GetFeatureInfo(feature::TypesHolder(ft), request);
}

bool VehicleModel::HasRoadType(FeatureType & ft) const
{
  return HasRoadType(feature::TypesHolder(ft));
}

bool VehicleModel::IsOneWay(FeatureType & ft) const
{
  for (uint32_t type : feature::TypesHolder(ft))
  {
    NormalizeType(type);
    if (IsOneWay(type) == ResultT::Yes)
      return true;
  }
  return false;
}

VehicleModel::Response VehicleModel::GetFeatureInfo(feature::TypesHolder const & types,
                                                    Request const & request) const
{
  Response response;

  SpeedFactor surface;
  Info const * info = nullptr;

  for (uint32_t type : types)
  {
    switch (GetRoadAvailability(type))
    {
    case ResultT::Yes:
      response.m_isValid = true;
      continue;
    case ResultT::No:
      response.m_isValid = false;
      return response;
    }

    switch (IsOneWay(type))
    {
    case ResultT::Yes:
      response.m_isOneWay = true;
      continue;
    case ResultT::No:
      response.m_isOneWay = false;
      continue;
    }

    auto const * sf = m_surfaceFactors.Find(type);
    if (sf)
    {
      surface = Pick<min>(*sf, surface);
      continue;
    }

    NormalizeType(type);

    if (info == nullptr)
    {
      info = m_highwayInfo.Find(type);
      if (info)
      {
        response.m_highwayType = static_cast<HighwayType>(type);
        response.m_isValid = true;
        response.m_isPassThroughAllowed = info->m_isPassThroughAllowed;
      }
    }
  }

  if (!response.m_isValid)
    return response;

  if (info == nullptr)
  {
//    LOG(LWARNING, ("Model doesn't have HighwayType entry, but hwtag=yes.",
//                   ft.DebugString(FeatureType::BEST_GEOMETRY)));

    response.m_isPassThroughAllowed = true;
  }

  response.m_forwardSpeed = GetSpeed(info, request.m_speed.GetSpeedKmPH(true), request.m_inCity) * surface;
  response.m_backwardSpeed = GetSpeed(info, request.m_speed.GetSpeedKmPH(false), request.m_inCity) * surface;
  return response;
}

double VehicleModel::GetMaxWeightSpeed() const
{
  return max(m_maxModelSpeed.m_inCity.m_weight, m_maxModelSpeed.m_outCity.m_weight);
}

void VehicleModel::NormalizeType(uint32_t type) const
{
  // The only exception that has 3-arity type.
  if (type != static_cast<uint32_t>(HighwayType::RailwayRailMotorVehicle))
    ftype::TruncValue(type, 2);
}

SpeedKMpH VehicleModel::GetSpeed(Info const * info, MaxspeedType ftMaxSpeed, bool inCity) const
{
  auto const maxModelSpeed = m_maxModelSpeed.GetSpeed(inCity);
  auto const factor = info ? info->m_factor.GetFactor(inCity) : 1.0;

  if (ftMaxSpeed != kInvalidSpeed)
  {
    return Pick<min>(SpeedKMpH(static_cast<double>(ftMaxSpeed)) * factor, maxModelSpeed);
  }
  else if (info)
  {
    SpeedKMpH const speed = info->m_speed.GetSpeed(inCity);

    /// @todo Hm, well this is not obvious for me and should be revised.
    // Predefined model speeds are taken from the former code and should not be multiplied with the factor.
    // On the contrary, ETA speed should be multiplied.
    return SpeedKMpH(min(speed.m_weight, maxModelSpeed.m_weight),
                     min(factor.m_eta * speed.m_eta, maxModelSpeed.m_eta));
  }
  else
  {
    return GetSpeedForAvailable();
  }
}


VehicleModelFactory::VehicleModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : m_countryParentNameGetterFn(countryParentNameGetterFn)
{
}

shared_ptr<VehicleModelInterface> VehicleModelFactory::GetVehicleModel() const
{
  auto const it = m_models.find("");
  CHECK(it != m_models.end(), ());
  return it->second;
}

shared_ptr<VehicleModelInterface>
VehicleModelFactory::GetVehicleModelForCountry(string const & country) const
{
  string parent = country;
  while (!parent.empty())
  {
    auto it = m_models.find(parent);
    if (it != m_models.end())
      return it->second;

    parent = GetParent(parent);
  }

  return GetVehicleModel();
}

string VehicleModelFactory::GetParent(string const & country) const
{
  if (!m_countryParentNameGetterFn)
    return string();
  return m_countryParentNameGetterFn(country);
}

HighwayFactors GetOneFactorsForBicycleAndPedestrianModel()
{
  return {
      {HighwayType::HighwayTrunk, InOutCityFactor(1.0)},
      {HighwayType::HighwayTrunkLink, InOutCityFactor(1.0)},
      {HighwayType::HighwayPrimary, InOutCityFactor(1.0)},
      {HighwayType::HighwayPrimaryLink, InOutCityFactor(1.0)},
      {HighwayType::HighwaySecondary, InOutCityFactor(1.0)},
      {HighwayType::HighwaySecondaryLink, InOutCityFactor(1.0)},
      {HighwayType::HighwayTertiary, InOutCityFactor(1.0)},
      {HighwayType::HighwayTertiaryLink, InOutCityFactor(1.0)},
      {HighwayType::HighwayService, InOutCityFactor(1.0)},
      {HighwayType::HighwayUnclassified, InOutCityFactor(1.0)},
      {HighwayType::HighwayRoad, InOutCityFactor(1.0)},
      {HighwayType::HighwayTrack, InOutCityFactor(1.0)},
      {HighwayType::HighwayPath, InOutCityFactor(1.0)},
      {HighwayType::HighwayBridleway, InOutCityFactor(1.0)},
      {HighwayType::HighwayCycleway, InOutCityFactor(1.0)},
      {HighwayType::HighwayResidential, InOutCityFactor(1.0)},
      {HighwayType::HighwayLivingStreet, InOutCityFactor(1.0)},
      {HighwayType::HighwaySteps, InOutCityFactor(1.0)},
      {HighwayType::HighwayPedestrian, InOutCityFactor(1.0)},
      {HighwayType::HighwayFootway, InOutCityFactor(1.0)},
      {HighwayType::ManMadePier, InOutCityFactor(1.0)},
      {HighwayType::RouteFerry, InOutCityFactor(1.0)},
  };
}

string DebugPrint(SpeedKMpH const & speed)
{
  ostringstream oss;
  oss << "SpeedKMpH [ ";
  oss << "weight:" << speed.m_weight << ", ";
  oss << "eta:" << speed.m_eta << " ]";
  return oss.str();
}

std::string DebugPrint(SpeedFactor const & speedFactor)
{
  ostringstream oss;
  oss << "SpeedFactor [ ";
  oss << "weight:" << speedFactor.m_weight << ", ";
  oss << "eta:" << speedFactor.m_eta << " ]";
  return oss.str();
}

string DebugPrint(InOutCitySpeedKMpH const & speed)
{
  ostringstream oss;
  oss << "InOutCitySpeedKMpH [ ";
  oss << "inCity:" << DebugPrint(speed.m_inCity) << ", ";
  oss << "outCity:" << DebugPrint(speed.m_outCity) << " ]";
  return oss.str();
}

string DebugPrint(InOutCityFactor const & speedFactor)
{
  ostringstream oss;
  oss << "InOutCityFactor [ ";
  oss << "inCity:" << DebugPrint(speedFactor.m_inCity) << ", ";
  oss << "outCity:" << DebugPrint(speedFactor.m_outCity) << " ]";
  return oss.str();
}

string DebugPrint(HighwayType type)
{
  switch (type)
  {
  case HighwayType::HighwayResidential: return "highway-residential";
  case HighwayType::HighwayService: return "highway-service";
  case HighwayType::HighwayUnclassified: return "highway-unclassified";
  case HighwayType::HighwayFootway: return "highway-footway";
  case HighwayType::HighwayTrack: return "highway-track";
  case HighwayType::HighwayTertiary: return "highway-tertiary";
  case HighwayType::HighwaySecondary: return "highway-secondary";
  case HighwayType::HighwayPath: return "highway-path";
  case HighwayType::HighwayPrimary: return "highway-primary";
  case HighwayType::HighwayRoad: return "highway-road";
  case HighwayType::HighwayCycleway: return "highway-cycleway";
  case HighwayType::HighwayMotorwayLink: return "highway-motorway_link";
  case HighwayType::HighwayLivingStreet: return "highway-living_street";
  case HighwayType::HighwayMotorway: return "highway-motorway";
  case HighwayType::HighwaySteps: return "highway-steps";
  case HighwayType::HighwayTrunk: return "highway-trunk";
  case HighwayType::HighwayPedestrian: return "highway-pedestrian";
  case HighwayType::HighwayTrunkLink: return "highway-trunk_link";
  case HighwayType::HighwayPrimaryLink: return "highway-primary_link";
  case HighwayType::ManMadePier: return "man_made-pier";
  case HighwayType::HighwayBridleway: return "highway-bridleway";
  case HighwayType::HighwaySecondaryLink: return "highway-secondary_link";
  case HighwayType::RouteFerry: return "route-ferry";
  case HighwayType::HighwayTertiaryLink: return "highway-tertiary_link";
  case HighwayType::RailwayRailMotorVehicle: return "railway-rail-motor_vehicle";
  case HighwayType::RouteShuttleTrain: return "route-shuttle_train";
  }

  UNREACHABLE();
}
}  // namespace routing
