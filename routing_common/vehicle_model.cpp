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

using namespace routing;
using namespace std;

namespace
{
template <double const & (*F)(double const &, double const &), typename WeightAndETA>
WeightAndETA Pick(WeightAndETA const & lhs, WeightAndETA const & rhs)
{
  return {F(lhs.m_weight, rhs.m_weight), F(lhs.m_eta, rhs.m_eta)};
};

InOutCitySpeedKMpH Max(InOutCitySpeedKMpH const & lhs, InOutCitySpeedKMpH const & rhs)
{
  return {Pick<max>(lhs.m_inCity, rhs.m_inCity), Pick<max>(lhs.m_outCity, rhs.m_outCity)};
}

/// @todo This function is used to to get key to fetch speed factor from model,
/// but we assign factors for links too. @see kHighwayBasedFactors.
HighwayType GetHighwayTypeKey(HighwayType type)
{
  switch (type)
  {
  case HighwayType::HighwayMotorwayLink: return HighwayType::HighwayMotorway;
  case HighwayType::HighwayTrunkLink: return HighwayType::HighwayTrunk;
  case HighwayType::HighwayPrimaryLink: return HighwayType::HighwayPrimary;
  case HighwayType::HighwaySecondaryLink: return HighwayType::HighwaySecondary;
  case HighwayType::HighwayTertiaryLink: return HighwayType::HighwayTertiary;
  default: return type;
  }
}
}  // namespace

namespace routing
{

VehicleModel::VehicleModel(Classificator const & classif, LimitsInitList const & featureTypeLimits,
                           SurfaceInitList const & featureTypeSurface, HighwayBasedInfo const & info)
: m_highwayBasedInfo(info), m_onewayType(classif.GetTypeByPath({"hwtag", "oneway"}))
{
  m_roadTypes.Reserve(featureTypeLimits.size());
  for (auto const & v : featureTypeLimits)
  {
    auto const clType = classif.GetTypeByPath(v.m_type);
    auto const hwType = static_cast<HighwayType>(classif.GetIndexForType(clType));
    auto const * speed = info.m_speeds.Find(hwType);
    ASSERT(speed, ("Can't found speed for", hwType));

    /// @todo Consider using not only highway class speed but max_speed * max_speed_factor.
    m_maxModelSpeed = Max(m_maxModelSpeed, *speed);
    m_roadTypes.Insert(clType, v.m_isPassThroughAllowed);
  }
  m_roadTypes.FinishBuilding();

  m_surfaceFactors.Reserve(featureTypeSurface.size());
  for (auto const & v : featureTypeSurface)
  {
    auto const & speedFactor = v.m_factor;
    ASSERT_LESS_OR_EQUAL(speedFactor.m_weight, 1.0, ());
    ASSERT_LESS_OR_EQUAL(speedFactor.m_eta, 1.0, ());
    ASSERT_GREATER(speedFactor.m_weight, 0.0, ());
    ASSERT_GREATER(speedFactor.m_eta, 0.0, ());
    m_surfaceFactors.Insert(classif.GetTypeByPath(v.m_type), v.m_factor);
  }
}

void VehicleModel::AddAdditionalRoadTypes(Classificator const & classif, AdditionalRoadsList const & roads)
{
  for (auto const & r : roads)
  {
    uint32_t const type = classif.GetTypeByPath(r.m_type);
    if (m_roadTypes.Find(type) == nullptr)
    {
      m_addRoadTypes.Insert(type, r.m_speed);
      m_maxModelSpeed = Max(m_maxModelSpeed, r.m_speed);
    }
  }
}

uint32_t VehicleModel::PrepareToMatchType(uint32_t type)
{
  return ftypes::BaseChecker::PrepareToMatch(type, 2);
}

SpeedKMpH VehicleModel::GetSpeed(FeatureType & f, SpeedParams const & speedParams) const
{
  feature::TypesHolder const types(f);

  RoadAvailability const restriction = GetRoadAvailability(types);
  if (restriction == RoadAvailability::NotAvailable || !HasRoadType(types))
    return {};

  return GetTypeSpeed(types, speedParams);
}

HighwayType VehicleModel::GetHighwayType(FeatureType & f) const
{
  feature::TypesHolder const types(f);
  for (auto const t : types)
  {
    auto const ret = GetHighwayType(t);
    if (ret)
      return *ret;

    if (m_addRoadTypes.Find(t))
      return static_cast<HighwayType>(classif().GetIndexForType(t));
  }

  UNREACHABLE();
  return HighwayType::HighwayResidential;
}

double VehicleModel::GetMaxWeightSpeed() const
{
  return max(m_maxModelSpeed.m_inCity.m_weight, m_maxModelSpeed.m_outCity.m_weight);
}

optional<HighwayType> VehicleModel::GetHighwayType(uint32_t type) const
{
  type = PrepareToMatchType(type);
  auto const * value = m_roadTypes.Find(type);
  if (value)
    return static_cast<HighwayType>(classif().GetIndexForType(type));
  return {};
}

void VehicleModel::GetSurfaceFactor(uint32_t type, SpeedFactor & factor) const
{
  auto const * surface = m_surfaceFactors.Find(type);
  if (surface)
    factor = Pick<min>(factor, *surface);

  ASSERT_LESS_OR_EQUAL(factor.m_weight, 1.0, ());
  ASSERT_LESS_OR_EQUAL(factor.m_eta, 1.0, ());
  ASSERT_GREATER(factor.m_weight, 0.0, ());
  ASSERT_GREATER(factor.m_eta, 0.0, ());
}

void VehicleModel::GetAdditionalRoadSpeed(uint32_t type, bool isCityRoad,
                                          optional<SpeedKMpH> & speed) const
{
  auto const * s = m_addRoadTypes.Find(type);
  if (s)
  {
    auto const & res = isCityRoad ? s->m_inCity : s->m_outCity;
    speed = speed ? Pick<min>(*speed, res) : res;
  }
}

SpeedKMpH VehicleModel::GetSpeedOnFeatureWithMaxspeed(HighwayType const & type,
                                                      SpeedParams const & speedParams) const
{
  ASSERT(speedParams.m_maxspeed.IsValid(), ());

  auto const featureMaxSpeedKmPH = speedParams.m_maxspeed.GetSpeedKmPH(speedParams.m_forward);
  ASSERT(featureMaxSpeedKmPH != kInvalidSpeed, (type, speedParams.m_forward, speedParams.m_maxspeed));

  auto const typeKey = GetHighwayTypeKey(type);
  auto const * factor = m_highwayBasedInfo.m_factors.Find(typeKey);
  ASSERT(factor, ("Key:", typeKey, "is not found."));

  bool const isCityRoad = speedParams.m_inCity;
  return Pick<min>(SpeedKMpH(static_cast<double>(featureMaxSpeedKmPH)) * factor->GetFactor(isCityRoad),
                   m_maxModelSpeed.GetSpeed(isCityRoad));
}

SpeedKMpH VehicleModel::GetSpeedOnFeatureWithoutMaxspeed(HighwayType const & type,
                                                         SpeedParams const & speedParams) const
{
  ASSERT(!speedParams.m_maxspeed.IsValid(), ());

  auto const * s = m_highwayBasedInfo.m_speeds.Find(type);
  ASSERT(s, ("Key:", type, "is not found."));

  auto const typeKey = GetHighwayTypeKey(type);
  auto const * factor = m_highwayBasedInfo.m_factors.Find(typeKey);
  ASSERT(factor, ("Key:", typeKey, "is not found."));

  auto const isCityRoad = speedParams.m_inCity;
  SpeedKMpH const speed = s->GetSpeed(isCityRoad);
  ASSERT(speed.IsValid(), (speed));

  SpeedKMpH const & maxModelSpeed = m_maxModelSpeed.GetSpeed(isCityRoad);

  /// @todo Hm, well this is not obvious for me and should be revised.
  // Speeds from the m_highwayBasedInfo are taken from the former code and should not
  // be multiplied to the factor. On the contrary, ETA speed should be multiplied.
  return SpeedKMpH(
      min(speed.m_weight, maxModelSpeed.m_weight),
      min(factor->GetFactor(isCityRoad).m_eta * speed.m_eta, maxModelSpeed.m_eta));
}

SpeedKMpH VehicleModel::GetTypeSpeed(feature::TypesHolder const & types,
                                     SpeedParams const & speedParams) const
{
  bool const isCityRoad = speedParams.m_inCity;
  optional<HighwayType> hwType;
  SpeedFactor surfaceFactor;
  optional<SpeedKMpH> additionalRoadSpeed;
  for (uint32_t t : types)
  {
    if (!hwType)
      hwType = GetHighwayType(t);

    GetSurfaceFactor(t, surfaceFactor);
    GetAdditionalRoadSpeed(t, isCityRoad, additionalRoadSpeed);
  }

  if (additionalRoadSpeed)
    return *additionalRoadSpeed * surfaceFactor;

  ASSERT(hwType, ());
  auto const resultHwType = *hwType;
  if (speedParams.m_maxspeed.IsValid())
    return GetSpeedOnFeatureWithMaxspeed(resultHwType, speedParams) * surfaceFactor;

  return GetSpeedOnFeatureWithoutMaxspeed(resultHwType, speedParams) * surfaceFactor;
}

SpeedKMpH VehicleModel::GetSpeedWihtoutMaxspeed(FeatureType & f,
                                                SpeedParams const & speedParams) const
{
  return VehicleModel::GetSpeed(f, {speedParams.m_forward, speedParams.m_inCity, Maxspeed()});
}

bool VehicleModel::IsOneWay(FeatureType & f) const
{
  return HasOneWayType(feature::TypesHolder(f));
}

bool VehicleModel::HasOneWayType(feature::TypesHolder const & types) const
{
  return types.Has(m_onewayType);
}

bool VehicleModel::IsRoad(FeatureType & f) const
{
  if (f.GetGeomType() != feature::GeomType::Line)
    return false;

  feature::TypesHolder const types(f);

  if (GetRoadAvailability(types) == RoadAvailability::NotAvailable)
    return false;
  return HasRoadType(types);
}

bool VehicleModel::IsPassThroughAllowed(FeatureType & f) const
{
  feature::TypesHolder const types(f);
  // Allow pass through additional road types e.g. peer, ferry.
  for (uint32_t t : types)
  {
    if (m_addRoadTypes.Find(t))
      return true;
  }
  return HasPassThroughType(types);
}

bool VehicleModel::HasPassThroughType(feature::TypesHolder const & types) const
{
  for (uint32_t const t : types)
  {
    bool const * allow = m_roadTypes.Find(PrepareToMatchType(t));
    if (allow && *allow)
      return true;
  }

  return false;
}

bool VehicleModel::IsRoadType(uint32_t type) const
{
  return m_addRoadTypes.Find(type) || m_roadTypes.Find(PrepareToMatchType(type));
}

VehicleModelInterface::RoadAvailability VehicleModel::GetRoadAvailability(feature::TypesHolder const &) const
{
  return RoadAvailability::Unknown;
}

VehicleModelFactory::VehicleModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : m_countryParentNameGetterFn(countryParentNameGetterFn)
{
}

shared_ptr<VehicleModelInterface> VehicleModelFactory::GetVehicleModel() const
{
  auto const itr = m_models.find("");
  ASSERT(itr != m_models.end(), ("No default vehicle model. VehicleModelFactory was not "
                                 "properly constructed"));
  return itr->second;
}

shared_ptr<VehicleModelInterface> VehicleModelFactory::GetVehicleModelForCountry(
    string const & country) const
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

HighwayBasedFactors GetOneFactorsForBicycleAndPedestrianModel()
{
  return HighwayBasedFactors{
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

string DebugPrint(VehicleModelInterface::RoadAvailability const l)
{
  switch (l)
  {
  case VehicleModelInterface::RoadAvailability::Available: return "Available";
  case VehicleModelInterface::RoadAvailability::NotAvailable: return "NotAvailable";
  case VehicleModelInterface::RoadAvailability::Unknown: return "Unknown";
  }

  UNREACHABLE();
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
