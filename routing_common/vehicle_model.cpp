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

VehicleModel::RoadLimits::RoadLimits(double speedKMpH, bool isPassThroughAllowed)
  : m_speedKMpH(speedKMpH), m_isPassThroughAllowed(isPassThroughAllowed)
{
  CHECK_GREATER(m_speedKMpH, 0.0, ());
}

VehicleModel::VehicleModel(Classificator const & c, LimitsInitList const & featureTypeLimits,
                           SurfaceInitList const & featureTypeSurface)
  : m_maxSpeedKMpH(0), m_onewayType(c.GetTypeByPath({"hwtag", "oneway"}))
{
  CHECK_EQUAL(m_surfaceFactors.size(), 4,
              ("If you want to change the size of the container please take into account that it's "
               "used with algorithm find() with linear complexity."));
  CHECK_EQUAL(featureTypeSurface.size(), m_surfaceFactors.size(), ());

  for (auto const & v : featureTypeLimits)
  {
    m_maxSpeedKMpH = max(m_maxSpeedKMpH, v.m_speedKMpH);
    m_highwayTypes.emplace(c.GetTypeByPath(vector<string>(v.m_types, v.m_types + 2)),
                           RoadLimits(v.m_speedKMpH, v.m_isPassThroughAllowed));
  }

  size_t i = 0;
  for (auto const & v : featureTypeSurface)
    m_surfaceFactors[i++] = {c.GetTypeByPath(vector<string>(v.m_types, v.m_types + 2)), v.m_speedFactor};
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
  // Decreasing speed factor based on road surface (cover).
  double speedFactor = 1.0;
  for (uint32_t t : types)
  {
    uint32_t const type = ftypes::BaseChecker::PrepareToMatch(t, 2);
    auto itHighway = m_highwayTypes.find(type);
    if (itHighway != m_highwayTypes.end())
      speed = min(speed, itHighway->second.GetSpeedKMpH());

    auto const addRoadInfoIter = FindRoadType(t);
    if (addRoadInfoIter != m_addRoadTypes.cend())
      speed = min(speed, addRoadInfoIter->m_speedKMpH);

    auto const itFactor = find_if(m_surfaceFactors.cbegin(), m_surfaceFactors.cend(),
                                  [t](TypeFactor const & v) { return v.m_type == t; });
    if (itFactor != m_surfaceFactors.cend())
      speedFactor = min(speedFactor, itFactor->m_factor);
  }
  
  if (speed <= m_maxSpeedKMpH && speedFactor <= 1.0)
    return speed * speedFactor;

  return 0.0 /* Speed */;
}

bool VehicleModel::IsOneWay(FeatureType const & f) const
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

bool VehicleModel::IsRoad(FeatureType const & f) const
{
  if (f.GetFeatureType() != feature::GEOM_LINE)
    return false;

  feature::TypesHolder const types(f);

  if (GetRoadAvailability(types) == RoadAvailability::NotAvailable)
    return false;
  return HasRoadType(types);
}

bool VehicleModel::IsPassThroughAllowed(FeatureType const & f) const
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
