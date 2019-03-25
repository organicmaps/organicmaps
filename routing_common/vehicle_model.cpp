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

#include <boost/optional.hpp>

using namespace routing;
using namespace std;

namespace
{
double constexpr kInvalidModelValue = -1.0;

template <double const & (*F)(double const &, double const &), typename WeightAndETA>
WeightAndETA Pick(WeightAndETA const & lhs, WeightAndETA const & rhs)
{
  return {F(lhs.m_weight, rhs.m_weight), F(lhs.m_eta, rhs.m_eta)};
};

InOutCitySpeedKMpH Max(InOutCitySpeedKMpH const & lhs, InOutCitySpeedKMpH const & rhs)
{
  return {Pick<max>(lhs.m_inCity, rhs.m_inCity), Pick<max>(lhs.m_outCity, rhs.m_outCity)};
}

MaxspeedType GetMaxspeedKey(double maxspeedValue)
{
  return base::asserted_cast<MaxspeedType>(10 * static_cast<MaxspeedType>((maxspeedValue + 5) / 10));
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
    auto const speedIt = info.m_globalSpeeds.find(highwayType);
    CHECK(speedIt != info.m_globalSpeeds.cend(), ("Can't found speed for", highwayType));
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

double VehicleModel::GetMaxWeightSpeed() const
{
  return max(m_maxModelSpeed.m_inCity.m_weight, m_maxModelSpeed.m_outCity.m_weight);
}

void VehicleModel::GetHighwayType(uint32_t type, boost::optional<HighwayType> & hwType) const
{
  if (hwType)
    return;

  type = ftypes::BaseChecker::PrepareToMatch(type, 2);
  auto const it = m_roadTypes.find(type);
  if (it != m_roadTypes.cend())
    hwType = it->second.GetHighwayType();
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

void VehicleModel::GetAdditionalRoadSpeed(uint32_t type, bool isCityRoad, boost::optional<SpeedKMpH> & speed) const
{
  auto const it = FindAdditionalRoadType(type);
  if (it == m_addRoadTypes.cend())
    return;

  auto const & res = isCityRoad ? it->m_speed.m_inCity : it->m_speed.m_outCity;
  speed = speed ? Pick<min>(*speed, res) : res;
}

SpeedKMpH VehicleModel::GetSpeedOnFeatureWithMaxspeed(boost::optional<HighwayType> const & type,
                                                      SpeedParams const & speedParams) const
{
  CHECK(speedParams.m_maxspeed.IsValid(), ());
  bool const isCityRoad = speedParams.m_inCity;
  SpeedKMpH const & maxModelSpeed = m_maxModelSpeed.GetSpeed(isCityRoad);
  auto const maxspeedKmPH = static_cast<double>(speedParams.m_maxspeed.GetSpeedKmPH(speedParams.m_forward));
  SpeedKMpH speed = Pick<min>(SpeedKMpH(maxspeedKmPH, maxspeedKmPH), maxModelSpeed);

  // Note. If a highway type is not found and |maxspeed| is set, |maxspeed| in kms per hour
  // should be returned. That means |maxspeedFactor| should be 1.0.
  if (!type)
    return speed;

  // We assume that all link roads are equal to its parents and drop "_link" suffix
  // while searching for the particular factor.
  auto const maxspeedKey = GetMaxspeedKey(maxspeedKmPH);
  SpeedFactor maxspeedFactor(kInvalidModelValue, kInvalidModelValue);
  auto const typeKey = GetHighwayTypeKey(*type);
  auto const local = m_highwayBasedInfo.m_localFactors.find(typeKey);
  if (local != m_highwayBasedInfo.m_localFactors.cend())
  {
    auto const & maxspeedsToFactors = local->second;
    auto const it = maxspeedsToFactors.find(maxspeedKey);
    if (it != maxspeedsToFactors.cend())
      maxspeedFactor = it->second.GetFactor(isCityRoad);
  }

  if (maxspeedFactor.m_weight != kInvalidModelValue && maxspeedFactor.m_eta != kInvalidModelValue)
    return Pick<min>(speed * maxspeedFactor, maxModelSpeed);

  auto const global = m_highwayBasedInfo.m_globalFactors.find(typeKey);
  if (global != m_highwayBasedInfo.m_globalFactors.cend())
  {
    auto const & maxspeedsToFactors = global->second;
    auto const it = maxspeedsToFactors.find(maxspeedKey);
    if (it != maxspeedsToFactors.cend())
    {
      auto const & factor = it->second.GetFactor(isCityRoad);
      if (factor.m_weight != kInvalidModelValue && maxspeedFactor.m_weight == kInvalidModelValue)
        maxspeedFactor.m_weight = factor.m_weight;

      if (factor.m_eta != kInvalidModelValue && maxspeedFactor.m_weight == kInvalidModelValue)
        maxspeedFactor.m_eta = factor.m_eta;
    }

    auto const defaultIt = maxspeedsToFactors.find(kCommonMaxSpeedValue);
    CHECK(defaultIt != maxspeedsToFactors.cend(), ());
    SpeedFactor const & defaultFactor = defaultIt->second.GetFactor(isCityRoad);
    if (maxspeedFactor.m_weight == kInvalidModelValue)
      maxspeedFactor.m_weight = defaultFactor.m_weight;

    if (maxspeedFactor.m_eta == kInvalidModelValue)
      maxspeedFactor.m_eta = defaultFactor.m_eta;
  }

  CHECK_NOT_EQUAL(maxspeedFactor.m_weight, kInvalidModelValue, ());
  CHECK_NOT_EQUAL(maxspeedFactor.m_eta, kInvalidModelValue, ());
  return Pick<min>(speed * maxspeedFactor, maxModelSpeed);
}

SpeedKMpH VehicleModel::GetSpeedOnFeatureWithoutMaxspeed(boost::optional<HighwayType> const & type,
                                                         SpeedParams const & speedParams) const
{
  CHECK(!speedParams.m_maxspeed.IsValid(), ());
  auto const isCityRoad = speedParams.m_inCity;
  if (!type)
    return m_maxModelSpeed.GetSpeed(isCityRoad);

  SpeedKMpH const & maxModelSpeed = m_maxModelSpeed.GetSpeed(isCityRoad);
  SpeedKMpH speed(kInvalidModelValue, kInvalidModelValue);
  auto const local = m_highwayBasedInfo.m_localSpeeds.find(*type);
  if (local != m_highwayBasedInfo.m_localSpeeds.cend())
    speed = local->second.GetSpeed(isCityRoad);

  if (speed.m_weight != kInvalidModelValue && speed.m_eta != kInvalidModelValue)
    return speed;

  auto const globalIt = m_highwayBasedInfo.m_globalSpeeds.find(*type);
  CHECK(globalIt != m_highwayBasedInfo.m_globalSpeeds.cend(), ("Can't find type in global speeds", *type));
  SpeedKMpH const & global = globalIt->second.GetSpeed(isCityRoad);
  CHECK_NOT_EQUAL(global.m_weight, kInvalidModelValue, ());
  CHECK_NOT_EQUAL(global.m_eta, kInvalidModelValue, ());
  if (speed.m_weight == kInvalidModelValue)
    speed.m_weight = global.m_weight;

  if (speed.m_eta == kInvalidModelValue)
    speed.m_eta = global.m_eta;

  return Pick<min>(speed, maxModelSpeed);
}

SpeedKMpH VehicleModel::GetTypeSpeed(feature::TypesHolder const & types,
                                     SpeedParams const & speedParams) const
{
  bool const isCityRoad = speedParams.m_inCity;
  boost::optional<HighwayType> hwType;
  SpeedFactor surfaceFactor;
  boost::optional<SpeedKMpH> additionalRoadSpeed;
  for (uint32_t t : types)
  {
    GetHighwayType(t, hwType);
    GetSurfaceFactor(t, surfaceFactor);
    GetAdditionalRoadSpeed(t, isCityRoad, additionalRoadSpeed);
  }

  if (additionalRoadSpeed)
    return *additionalRoadSpeed * surfaceFactor;

  if (speedParams.m_maxspeed.IsValid())
    return GetSpeedOnFeatureWithMaxspeed(hwType, speedParams) * surfaceFactor;

  return GetSpeedOnFeatureWithoutMaxspeed(hwType, speedParams) * surfaceFactor;
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
  if (f.GetFeatureType() != feature::GEOM_LINE)
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
    {
      LOG(LDEBUG, ("Vehicle model for", country, " was found:", parent));
      return it->second;
    }
    parent = GetParent(parent);
  }

  LOG(LDEBUG, ("Vehicle model wasn't found, default model is used instead:", country));
  return GetVehicleModel();
}

string VehicleModelFactory::GetParent(string const & country) const
{
  if (!m_countryParentNameGetterFn)
    return string();
  return m_countryParentNameGetterFn(country);
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

string DebugPrint(InOutCitySpeedKMpH const & speed)
{
  ostringstream oss;
  oss << "InOutCitySpeedKMpH [ ";
  oss << "inCity:" << DebugPrint(speed.m_inCity) << ", ";
  oss << "outCity:" << DebugPrint(speed.m_outCity) << " ]";
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
  case HighwayType::HighwayPlatform: return "highway-platform";
  case HighwayType::RouteShuttleTrain: return "route-shuttle_train";
  }
}
}  // namespace routing
