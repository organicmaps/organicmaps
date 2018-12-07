#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

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

VehicleModel::InOutCitySpeedKMpH Max(VehicleModel::InOutCitySpeedKMpH const & lhs,
                                     VehicleModel::InOutCitySpeedKMpH const & rhs)
{
  using MaxspeedFactor = VehicleModel::MaxspeedFactor;
  return {Pick<max>(lhs.m_inCity, rhs.m_inCity), Pick<max>(lhs.m_outCity, rhs.m_outCity),
          MaxspeedFactor(min(lhs.m_maxspeedFactor.m_weight, rhs.m_maxspeedFactor.m_weight),
                         min(lhs.m_maxspeedFactor.m_eta, rhs.m_maxspeedFactor.m_eta))};
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

VehicleModel::RoadLimits::RoadLimits(VehicleModel::InOutCitySpeedKMpH const & speed,
                                     bool isPassThroughAllowed)
  : m_speed(speed), m_isPassThroughAllowed(isPassThroughAllowed)
{
  CHECK_GREATER(m_speed.m_inCity.m_weight, 0.0, ());
  CHECK_GREATER(m_speed.m_inCity.m_eta, 0.0, ());
  CHECK_GREATER(m_speed.m_outCity.m_weight, 0.0, ());
  CHECK_GREATER(m_speed.m_outCity.m_eta, 0.0, ());
}

VehicleModel::VehicleModel(Classificator const & c, LimitsInitList const & featureTypeLimits,
                           SurfaceInitList const & featureTypeSurface)
  : m_modelMaxSpeed({}, {}, MaxspeedFactor(1.0)), m_onewayType(c.GetTypeByPath({"hwtag", "oneway"}))
{
  CHECK_EQUAL(m_surfaceFactors.size(), 4,
              ("If you want to change the size of the container please take into account that it's "
               "used with algorithm find() with linear complexity."));
  CHECK_EQUAL(featureTypeSurface.size(), m_surfaceFactors.size(), ());

  for (auto const & v : featureTypeLimits)
  {
    m_modelMaxSpeed = Max(m_modelMaxSpeed, v.m_speed);
    m_highwayTypes.emplace(c.GetTypeByPath(v.m_types),
                           RoadLimits(v.m_speed, v.m_isPassThroughAllowed));
  }

  size_t i = 0;
  for (auto const & v : featureTypeSurface)
  {
    auto const & speedFactor = v.m_factor;
    CHECK_LESS_OR_EQUAL(speedFactor.m_weight, 1.0, ());
    CHECK_LESS_OR_EQUAL(speedFactor.m_eta, 1.0, ());
    CHECK_GREATER_OR_EQUAL(speedFactor.m_weight, 0.0, ());
    CHECK_GREATER_OR_EQUAL(speedFactor.m_eta, 0.0, ());
    double const weightFactor = base::clamp(speedFactor.m_weight, 0.0, 1.0);
    double const etaFactor = base::clamp(speedFactor.m_eta, 0.0, 1.0);
    m_surfaceFactors[i++] = {c.GetTypeByPath(v.m_types), {weightFactor, etaFactor}};
  }
}

void VehicleModel::SetAdditionalRoadTypes(Classificator const & c,
                                          vector<AdditionalRoadTags> const & additionalTags)
{
  for (auto const & tag : additionalTags)
  {
    m_addRoadTypes.emplace_back(c, tag);
    m_modelMaxSpeed = Max(m_modelMaxSpeed, tag.m_speed);
  }
}

VehicleModel::SpeedKMpH VehicleModel::GetSpeed(FeatureType & f, SpeedParams const & speedParams) const
{
  feature::TypesHolder const types(f);

  RoadAvailability const restriction = GetRoadAvailability(types);
  // @TODO(bykoianko) Consider using speed on feature |f| instead of using max speed below.
  if (restriction == RoadAvailability::Available)
    return speedParams.m_inCity ? m_modelMaxSpeed.m_inCity : m_modelMaxSpeed.m_outCity;
  if (restriction != RoadAvailability::NotAvailable && HasRoadType(types))
    return GetTypeSpeed(types, speedParams);

  return {};
}

double VehicleModel::GetMaxWeightSpeed() const
{
  return max(m_modelMaxSpeed.m_inCity.m_weight, m_modelMaxSpeed.m_outCity.m_weight);
}

VehicleModel::SpeedKMpH VehicleModel::GetTypeSpeed(feature::TypesHolder const & types,
                                                   SpeedParams const & speedParams) const
{
  double const maxSpeedWeight =
      speedParams.m_inCity ? m_modelMaxSpeed.m_inCity.m_weight : m_modelMaxSpeed.m_outCity.m_weight;
  double const maxEtaWeight =
      speedParams.m_inCity ? m_modelMaxSpeed.m_inCity.m_eta : m_modelMaxSpeed.m_outCity.m_eta;

  // Note. If a highway type is not found and |maxspeed| is set, |maxspeed| in kms per hour
  // should be returned. That means |maxspeedFactor| should be 1.0.
  MaxspeedFactor maxspeedFactor(1.0);
  VehicleModel::SpeedKMpH speed{maxSpeedWeight * 2.0, maxEtaWeight * 2.0};
  for (uint32_t t : types)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareToMatch(t, 2);
    auto const itHighway = m_highwayTypes.find(type);
    if (itHighway != m_highwayTypes.cend())
    {
      speed = itHighway->second.GetSpeed(speedParams.m_inCity);
      maxspeedFactor = itHighway->second.GetMaxspeedFactor();
      break;
    }
  }

  if (speedParams.m_maxspeed.IsValid())
  {
    double const maxspeedKmPH =
        static_cast<double>(speedParams.m_maxspeed.GetSpeedKmPH(speedParams.m_forward));
    auto const weightSpeedKmPH =
        min(static_cast<double>(maxspeedFactor.m_weight * maxspeedKmPH), GetMaxWeightSpeed());
    return SpeedKMpH(weightSpeedKmPH, maxspeedFactor.m_eta * maxspeedKmPH);
  }

  // Decreasing speed factor based on road surface (cover).
  VehicleModel::SpeedFactor factor;
  for (uint32_t t : types)
  {
    auto const addRoadInfoIter = FindRoadType(t);
    if (addRoadInfoIter != m_addRoadTypes.cend())
    {
      speed = Pick<min>(
          speed, speedParams.m_inCity ? addRoadInfoIter->m_speed.m_inCity : addRoadInfoIter->m_speed.m_outCity);
    }

    auto const itFactor = find_if(m_surfaceFactors.cbegin(), m_surfaceFactors.cend(),
                                  [t](TypeFactor const & v) { return v.m_type == t; });
    if (itFactor != m_surfaceFactors.cend())
      factor = Pick<min>(factor, itFactor->m_factor);
  }

  CHECK_LESS_OR_EQUAL(factor.m_weight, 1.0, ());
  CHECK_LESS_OR_EQUAL(factor.m_eta, 1.0, ());
  CHECK_GREATER_OR_EQUAL(factor.m_weight, 0.0, ());
  CHECK_GREATER_OR_EQUAL(factor.m_eta, 0.0, ());

  VehicleModel::SpeedKMpH ret;
  if (speed.m_weight <= maxSpeedWeight)
    ret.m_weight = speed.m_weight * factor.m_weight;

  if (speed.m_eta <= maxEtaWeight)
    ret.m_eta = speed.m_eta * factor.m_eta;

  return ret;
}

VehicleModel::SpeedKMpH VehicleModel::GetSpeedWihtoutMaxspeed(FeatureType & f,
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
    auto const addRoadInfoIter = FindRoadType(t);
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
    auto it = m_highwayTypes.find(type);
    if (it != m_highwayTypes.end() && it->second.IsPassThroughAllowed())
      return true;
  }

  return false;
}

bool VehicleModel::IsRoadType(uint32_t type) const
{
  return FindRoadType(type) != m_addRoadTypes.cend() ||
         m_highwayTypes.find(ftypes::BaseChecker::PrepareToMatch(type, 2)) != m_highwayTypes.end();
}

VehicleModelInterface::RoadAvailability VehicleModel::GetRoadAvailability(feature::TypesHolder const & /* types */) const
{
  return RoadAvailability::Unknown;
}

vector<VehicleModel::AdditionalRoadType>::const_iterator VehicleModel::FindRoadType(
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

double GetMaxWeight(VehicleModel::InOutCitySpeedKMpH const & speed)
{
  return max(speed.m_inCity.m_weight, speed.m_outCity.m_weight);
}

string DebugPrint(VehicleModelInterface::RoadAvailability const l)
{
  switch (l)
  {
  case VehicleModelInterface::RoadAvailability::Available: return "Available";
  case VehicleModelInterface::RoadAvailability::NotAvailable: return "NotAvailable";
  case VehicleModelInterface::RoadAvailability::Unknown: return "Unknown";
  }

  ostringstream out;
  out << "Unknown VehicleModelInterface::RoadAvailability (" << static_cast<int>(l) << ")";
  return out.str();
}

std::string DebugPrint(VehicleModelInterface::SpeedKMpH const & speed)
{
  ostringstream oss;
  oss << "SpeedKMpH [ ";
  oss << "weight:" << speed.m_weight << ", ";
  oss << "eta:" << speed.m_eta << " ]";
  return oss.str();
}

std::string DebugPrint(VehicleModelInterface::InOutCitySpeedKMpH const & speed)
{
  ostringstream oss;
  oss << "InOutCitySpeedKMpH [ ";
  oss << "inCity:" << DebugPrint(speed.m_inCity) << ", ";
  oss << "outCity:" << DebugPrint(speed.m_outCity) << " ]";
  return oss.str();
}
}  // namespace routing
