#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <sstream>

namespace routing
{
using namespace std;

namespace
{
template <double const & (*F)(double const &, double const &), typename WeightAndETA>
WeightAndETA Pick(WeightAndETA const & lhs, WeightAndETA const & rhs)
{
  return {F(lhs.m_weight, rhs.m_weight), F(lhs.m_eta, rhs.m_eta)};
};

SpeedKMpH Max(SpeedKMpH const & lhs, InOutCitySpeedKMpH const & rhs)
{
  return Pick<max>(lhs, Pick<max>(rhs.m_inCity, rhs.m_outCity));
}
}  // namespace

VehicleModel::VehicleModel(Classificator const & classif, LimitsInitList const & featureTypeLimits,
                           SurfaceInitList const & featureTypeSurface, HighwayBasedInfo const & info)
: m_highwayBasedInfo(info)
, m_onewayType(ftypes::IsOneWayChecker::Instance().GetType())
{
  m_roadTypes.Reserve(featureTypeLimits.size());
  for (auto const & v : featureTypeLimits)
  {
    auto const * speed = info.m_speeds.Find(v.m_type);
    ASSERT(speed, ("Can't found speed for", v.m_type));

    m_maxModelSpeed = Max(m_maxModelSpeed, *speed);

    m_roadTypes.Insert(classif.GetTypeForIndex(static_cast<uint32_t>(v.m_type)), v.m_isPassThroughAllowed);
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
    m_surfaceFactors.Insert(classif.GetTypeByPath(v.m_type), speedFactor);
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

std::optional<HighwayType> VehicleModel::GetHighwayType(FeatureType & f) const
{
  feature::TypesHolder const types(f);
  for (uint32_t t : types)
  {
    ftype::TruncValue(t, 2);

    auto const ret = GetHighwayType(t);
    if (ret)
      return *ret;
  }

  // For example Denmark has "No track" profile (see kCarOptionsDenmark), but tracks exist in MWM.
  return {};
}

double VehicleModel::GetMaxWeightSpeed() const
{
  return m_maxModelSpeed.m_weight;
}

optional<HighwayType> VehicleModel::GetHighwayType(uint32_t type) const
{
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
    // Now we have only 1 additional type "yes" for all models.
    ASSERT(!speed, ());
    speed = isCityRoad ? s->m_inCity : s->m_outCity;
  }
}

/// @note Saved speed |params| is taken into account only if isCar == true.
SpeedKMpH VehicleModel::GetTypeSpeedImpl(feature::TypesHolder const & types, SpeedParams const & params, bool isCar) const
{
  bool const isCityRoad = params.m_inCity;
  optional<HighwayType> hwType;
  SpeedFactor surfaceFactor;
  optional<SpeedKMpH> additionalRoadSpeed;
  for (uint32_t t : types)
  {
    ftype::TruncValue(t, 2);

    if (!hwType)
      hwType = GetHighwayType(t);

    GetSurfaceFactor(t, surfaceFactor);
    GetAdditionalRoadSpeed(t, isCityRoad, additionalRoadSpeed);
  }

  SpeedKMpH speed;
  if (hwType)
  {
    if (isCar && params.m_maxspeed.IsValid())
    {
      MaxspeedType const s = params.m_maxspeed.GetSpeedKmPH(params.m_forward);
      ASSERT(s != kInvalidSpeed, (*hwType, params.m_forward, params.m_maxspeed));
      speed = {static_cast<double>(s)};
    }
    else
    {
      auto const * s = m_highwayBasedInfo.m_speeds.Find(*hwType);
      ASSERT(s, ("Key:", *hwType, "is not found."));
      speed = s->GetSpeed(isCityRoad);

      if (isCar)
      {
        // Override the global default speed with the MWM's saved default speed if they are not significantly differ (2x),
        // to avoid anomaly peaks (especially for tracks).
        /// @todo MWM saved speeds should be validated in generator.
        if (params.m_defSpeedKmPH != kInvalidSpeed &&
            fabs(speed.m_weight - params.m_defSpeedKmPH) / speed.m_weight < 1.0)
        {
          double const factor = speed.m_eta / speed.m_weight;
          speed.m_weight = params.m_defSpeedKmPH;
          speed.m_eta = speed.m_weight * factor;
        }
      }
      else
      {
        // Take additional speed for bicycle and pedestrian only.
        if (additionalRoadSpeed)
          speed = Pick<max>(speed, *additionalRoadSpeed);
      }
    }

    auto const typeKey = *hwType;
    auto const * factor = m_highwayBasedInfo.m_factors.Find(typeKey);
    ASSERT(factor, ("Key:", typeKey, "is not found."));
    auto const & f = factor->GetFactor(isCityRoad);
    speed.m_weight *= f.m_weight;
    speed.m_eta *= f.m_eta;
  }
  else
  {
    // In case if we have highway=footway + motorcar=yes :)
    ASSERT(additionalRoadSpeed, ());
    if (additionalRoadSpeed)
      speed = *additionalRoadSpeed;
  }

  ASSERT(!(m_maxModelSpeed < speed), (speed, m_maxModelSpeed, types));
  return speed * surfaceFactor;
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
  return f.GetGeomType() == feature::GeomType::Line && IsRoadImpl(feature::TypesHolder(f));
}

bool VehicleModel::IsPassThroughAllowed(FeatureType & f) const
{
  return HasPassThroughType(feature::TypesHolder(f));
}

bool VehicleModel::HasPassThroughType(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    ftype::TruncValue(t, 2);

    // Additional types (like ferry) are always pass-through now.
    if (m_addRoadTypes.Find(t))
      return true;

    bool const * allow = m_roadTypes.Find(t);
    if (allow && *allow)
      return true;
  }

  return false;
}

bool VehicleModel::IsRoadType(uint32_t type) const
{
  ftype::TruncValue(type, 2);
  return m_addRoadTypes.Find(type) || m_roadTypes.Find(type);
}

bool VehicleModel::IsRoadImpl(feature::TypesHolder const & types) const
{
  for (uint32_t const t : types)
  {
    // Assume that Yes and No are not possible at the same time. Return first flag, otherwise.
    if (t == m_yesType)
      return true;
    if (t == m_noType)
      return false;
  }

  return HasRoadType(types);
}

VehicleModelFactory::VehicleModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : m_countryParentNameGetterFn(countryParentNameGetterFn)
{
}

shared_ptr<VehicleModelInterface> VehicleModelFactory::GetVehicleModel() const
{
  auto const itr = m_models.find("");
  ASSERT(itr != m_models.end(), ());
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
  case HighwayType::HighwayBusway: return "highway-busway";
  case HighwayType::RouteShuttleTrain: return "route-shuttle_train";
  }

  UNREACHABLE();
}
}  // namespace routing
