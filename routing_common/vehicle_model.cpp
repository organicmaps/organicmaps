#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/macros.hpp"

#include <algorithm>

using namespace std;

namespace routing
{
VehicleModel::AdditionalRoadType::AdditionalRoadType(Classificator const & c,
                                                     AdditionalRoadTags const & tag)
  : m_type(c.GetTypeByPath(tag.m_hwtag)), m_speedKMpH(tag.m_speedKMpH)
{
}

VehicleModel::RoadLimits::RoadLimits(double speedKMpH, bool isTransitAllowed)
  : m_speedKMpH(speedKMpH), m_isTransitAllowed(isTransitAllowed)
{
  CHECK_GREATER(m_speedKMpH, 0.0, ());
}

VehicleModel::VehicleModel(Classificator const & c, InitListT const & featureTypeLimits)
  : m_maxSpeedKMpH(0),
    m_onewayType(c.GetTypeByPath({ "hwtag", "oneway" }))
{
  for (auto const & v : featureTypeLimits)
  {
    m_maxSpeedKMpH = max(m_maxSpeedKMpH, v.m_speedKMpH);
    m_types.emplace(c.GetTypeByPath(vector<string>(v.m_types, v.m_types + 2)),
                    RoadLimits(v.m_speedKMpH, v.m_isTransitAllowed));
  }
}

void VehicleModel::SetAdditionalRoadTypes(Classificator const & c,
                                          vector<AdditionalRoadTags> const & additionalTags)
{
  for (auto const & tag : additionalTags)
  {
    m_addRoadTypes.emplace_back(c, tag);
    m_maxSpeedKMpH = max(m_maxSpeedKMpH, tag.m_speedKMpH);
  }
}

double VehicleModel::GetSpeed(FeatureType const & f) const
{
  feature::TypesHolder const types(f);

  RoadAvailability const restriction = GetRoadAvailability(types);
  if (restriction == RoadAvailability::Available)
    return GetMaxSpeed();
  if (restriction != RoadAvailability::NotAvailable && HasRoadType(types))
    return GetMinTypeSpeed(types);

  return 0.0 /* Speed */;
}

double VehicleModel::GetMinTypeSpeed(feature::TypesHolder const & types) const
{
  double speed = m_maxSpeedKMpH * 2;
  for (uint32_t t : types)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareToMatch(t, 2);
    auto it = m_types.find(type);
    if (it != m_types.end())
      speed = min(speed, it->second.GetSpeedKMpH());

    auto const addRoadInfoIter = FindRoadType(t);
    if (addRoadInfoIter != m_addRoadTypes.cend())
      speed = min(speed, addRoadInfoIter->m_speedKMpH);
  }
  if (speed <= m_maxSpeedKMpH)
    return speed;

  return 0.0 /* Speed */;
}

bool VehicleModel::IsOneWay(FeatureType const & f) const
{
  // It's a hotfix for release and this code shouldn't be merge to master.
  // According to osm documentation on roundabout it's implied that roundabout is one way
  // road execpt for rare cases. Only 0.3% (~1200) of roundabout in the world are two-way road.
  // (http://wiki.openstreetmap.org/wiki/Tag:junction%3Droundabout)
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

bool VehicleModel::IsRoad(FeatureType const & f) const
{
  if (f.GetFeatureType() != feature::GEOM_LINE)
    return false;

  feature::TypesHolder const types(f);

  if (GetRoadAvailability(types) == RoadAvailability::NotAvailable)
    return false;
  return HasRoadType(types);
}

bool VehicleModel::IsTransitAllowed(FeatureType const & f) const
{
  feature::TypesHolder const types(f);
  return HasTransitType(types);
}

bool VehicleModel::HasTransitType(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareToMatch(t, 2);
    auto it = m_types.find(type);
    if (it != m_types.end() && it->second.IsTransitAllowed())
      return true;
  }

  return false;
}

bool VehicleModel::IsRoadType(uint32_t type) const
{
  return FindRoadType(type) != m_addRoadTypes.cend() ||
         m_types.find(ftypes::BaseChecker::PrepareToMatch(type, 2)) != m_types.end();
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

string DebugPrint(VehicleModelInterface::RoadAvailability const l)
{
  switch (l)
  {
  case VehicleModelInterface::RoadAvailability::Available: return "Available";
  case VehicleModelInterface::RoadAvailability::NotAvailable: return "NotAvailable";
  case VehicleModelInterface::RoadAvailability::Unknown: return "Unknown";
  }

  stringstream out;
  out << "Unknown VehicleModelInterface::RoadAvailability (" << static_cast<int>(l) << ")";
  return out.str();
}
}  // namespace routing
