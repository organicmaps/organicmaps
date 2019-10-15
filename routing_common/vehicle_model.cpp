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
VehicleModel::AdditionalRoadType::AdditionalRoadType(Classificator const & c,
                                                     AdditionalRoadTags const & tag)
  : m_type(c.GetTypeByPath(tag.m_hwtag))
  , m_speed(tag.m_speed)
{
}

VehicleModel::VehicleModel(Classificator const & c, LimitsInitList const & featureTypeLimits,
                           SurfaceInitList const & featureTypeSurface,
                           HighwayBasedInfo const & info)
  : m_onewayType(c.GetTypeByPath({"hwtag", "oneway"})), m_highwayBasedInfo(info)
{
  CHECK_EQUAL(m_surfaceFactors.size(), 4,
              ("If you want to change the size of the container please take into account that it's "
               "used with algorithm find() with linear complexity."));
  CHECK_EQUAL(featureTypeSurface.size(), m_surfaceFactors.size(), ());

  for (auto const & v : featureTypeLimits)
  {
    auto const classificatorType = c.GetTypeByPath(v.m_types);
    auto const highwayType = static_cast<HighwayType>(c.GetIndexForType(classificatorType));
    auto const speedIt = info.m_speeds.find(highwayType);
    CHECK(speedIt != info.m_speeds.cend(), ("Can't found speed for", highwayType));
    // TODO: Consider using not only highway class speed but max sped * max speed factor.
    m_maxModelSpeed = Max(m_maxModelSpeed, speedIt->second);
    m_roadTypes.emplace(classificatorType, RoadType(highwayType, v.m_isPassThroughAllowed));
  }

  size_t i = 0;
  for (auto const & v : featureTypeSurface)
  {
    auto const & speedFactor = v.m_factor;
    CHECK_LESS_OR_EQUAL(speedFactor.m_weight, 1.0, ());
    CHECK_LESS_OR_EQUAL(speedFactor.m_eta, 1.0, ());
    CHECK_GREATER(speedFactor.m_weight, 0.0, ());
    CHECK_GREATER(speedFactor.m_eta, 0.0, ());
    m_surfaceFactors[i++] = {c.GetTypeByPath(v.m_types), v.m_factor};
  }
}

void VehicleModel::SetAdditionalRoadTypes(Classificator const & c,
                                          vector<AdditionalRoadTags> const & additionalTags)
{
  for (auto const & tag : additionalTags)
  {
    m_addRoadTypes.emplace_back(c, tag);
    m_maxModelSpeed = Max(m_maxModelSpeed, tag.m_speed);
  }
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
  optional<HighwayType> ret;
  for (auto const t : types)
  {
    ret = GetHighwayType(t);
    if (ret)
      break;

    if (FindAdditionalRoadType(t) != m_addRoadTypes.end())
      return static_cast<HighwayType>(classif().GetIndexForType(t));
  }

  return *ret;
}

double VehicleModel::GetMaxWeightSpeed() const
{
  return max(m_maxModelSpeed.m_inCity.m_weight, m_maxModelSpeed.m_outCity.m_weight);
}

optional<HighwayType> VehicleModel::GetHighwayType(uint32_t type) const
{
  optional<HighwayType> hwType;
  type = ftypes::BaseChecker::PrepareToMatch(type, 2);
  auto const it = m_roadTypes.find(type);
  if (it != m_roadTypes.cend())
    hwType = it->second.GetHighwayType();

  return hwType;
}

void VehicleModel::GetSurfaceFactor(uint32_t type, SpeedFactor & factor) const
{
  auto const it = find_if(m_surfaceFactors.cbegin(), m_surfaceFactors.cend(),
                          [type](TypeFactor const & v) { return v.m_type == type; });

  if (it != m_surfaceFactors.cend())
    factor = Pick<min>(factor, it->m_factor);

  CHECK_LESS_OR_EQUAL(factor.m_weight, 1.0, ());
  CHECK_LESS_OR_EQUAL(factor.m_eta, 1.0, ());
  CHECK_GREATER(factor.m_weight, 0.0, ());
  CHECK_GREATER(factor.m_eta, 0.0, ());
}

void VehicleModel::GetAdditionalRoadSpeed(uint32_t type, bool isCityRoad,
                                          optional<SpeedKMpH> & speed) const
{
  auto const it = FindAdditionalRoadType(type);
  if (it == m_addRoadTypes.cend())
    return;

  auto const & res = isCityRoad ? it->m_speed.m_inCity : it->m_speed.m_outCity;
  speed = speed ? Pick<min>(*speed, res) : res;
}

SpeedKMpH VehicleModel::GetSpeedOnFeatureWithMaxspeed(HighwayType const & type,
                                                      SpeedParams const & speedParams) const
{
  ASSERT(speedParams.m_maxspeed.IsValid(), ());
  bool const isCityRoad = speedParams.m_inCity;
  auto const featureMaxSpeedKmPH = speedParams.m_maxspeed.GetSpeedKmPH(speedParams.m_forward);
  ASSERT(featureMaxSpeedKmPH != kInvalidSpeed, (type, speedParams.m_forward, speedParams.m_maxspeed));

  // We assume that all link roads are equal to its parents and drop "_link" suffix
  // while searching for the particular factor.
  auto const highwayType = GetHighwayTypeKey(type);

  auto const factorIt = m_highwayBasedInfo.m_factors.find(highwayType);
  ASSERT(factorIt != m_highwayBasedInfo.m_factors.cend(), ("Key:", highwayType, "is not found."));
  auto const & factor = factorIt->second;
  SpeedKMpH const & maxModelSpeed = m_maxModelSpeed.GetSpeed(isCityRoad);
  return Pick<min>(SpeedKMpH(static_cast<double>(featureMaxSpeedKmPH)) * factor.GetFactor(isCityRoad),
                   maxModelSpeed);
}

SpeedKMpH VehicleModel::GetSpeedOnFeatureWithoutMaxspeed(HighwayType const & type,
                                                         SpeedParams const & speedParams) const
{
  ASSERT(!speedParams.m_maxspeed.IsValid(), ());
  auto const isCityRoad = speedParams.m_inCity;
  SpeedKMpH const & maxModelSpeed = m_maxModelSpeed.GetSpeed(isCityRoad);

  auto const speedIt = m_highwayBasedInfo.m_speeds.find(type);
  ASSERT(speedIt != m_highwayBasedInfo.m_speeds.cend(), ("Key:", type, "is not found."));

  auto const typeKey = GetHighwayTypeKey(type);
  auto const factorIt = m_highwayBasedInfo.m_factors.find(typeKey);
  ASSERT(factorIt != m_highwayBasedInfo.m_factors.cend(), ("Key:", typeKey, "is not found."));

  SpeedKMpH const speed = speedIt->second.GetSpeed(isCityRoad);
  ASSERT(speed.IsValid(), (speed));
  // Note. Weight speeds put to |m_highwayBasedInfo| are taken from the former code and should not
  // be multiplied to the factor. On the contrary eta speed should be multiplied.
  return SpeedKMpH(
      min(speed.m_weight, maxModelSpeed.m_weight),
      min(factorIt->second.GetFactor(isCityRoad).m_eta * speed.m_eta, maxModelSpeed.m_eta));
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
  // It's a hotfix for release and this code shouldn't be merge to master.
  // According to osm documentation on roundabout it's implied that roundabout is one way
  // road execpt for rare cases. Only 0.3% (~1200) of roundabout in the world are two-way road.
  // (https://wiki.openstreetmap.org/wiki/Tag:junction%3Droundabout)
  // It should be processed on map generation stage together with other implied one way features
  // rules like: motorway_link (if not set oneway == "no")
  // motorway (if not set oneway == "no"). Please see
  // https://github.com/mapsme/omim/blob/master/3party/osrm/osrm-backend/profiles/car.lua#L392
  // for further details.
  // TODO(@Zverik, @bykoianko) Please process the rules on map generation stage.
  return HasOneWayType(feature::TypesHolder(f)) || ftypes::IsRoundAboutChecker::Instance()(f);
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
    auto const addRoadInfoIter = FindAdditionalRoadType(t);
    if (addRoadInfoIter != m_addRoadTypes.cend())
      return true;
  }
  return HasPassThroughType(types);
}

bool VehicleModel::HasPassThroughType(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareToMatch(t, 2);
    auto it = m_roadTypes.find(type);
    if (it != m_roadTypes.end() && it->second.IsPassThroughAllowed())
      return true;
  }

  return false;
}

bool VehicleModel::IsRoadType(uint32_t type) const
{
  return FindAdditionalRoadType(type) != m_addRoadTypes.cend() ||
         m_roadTypes.find(ftypes::BaseChecker::PrepareToMatch(type, 2)) != m_roadTypes.end();
}

VehicleModelInterface::RoadAvailability VehicleModel::GetRoadAvailability(feature::TypesHolder const & /* types */) const
{
  return RoadAvailability::Unknown;
}

vector<VehicleModel::AdditionalRoadType>::const_iterator VehicleModel::FindAdditionalRoadType(
    uint32_t type) const
{
  return find_if(m_addRoadTypes.begin(), m_addRoadTypes.cend(),
                 [&type](AdditionalRoadType const & t) { return t.m_type == type; });
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
  case HighwayType::RouteFerryMotorcar: return "route-ferry-motorcar";
  case HighwayType::RouteFerryMotorVehicle: return "route-ferry-motor_vehicle";
  case HighwayType::RailwayRailMotorVehicle: return "railway-rail-motor_vehicle";
  case HighwayType::RouteShuttleTrain: return "route-shuttle_train";
  }

  UNREACHABLE();
}
}  // namespace routing
