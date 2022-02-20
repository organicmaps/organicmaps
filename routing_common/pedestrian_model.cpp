#include "routing_common/pedestrian_model.hpp"

#include "indexer/classificator.hpp"

namespace pedestrian_model
{
using namespace routing;

// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions
// Document contains proposals for some countries, but we assume that some kinds of roads are ready for pedestrian routing,
// but not listed in tables in the document. For example, steps are not listed, paths, roads and services features also
// can be treated as ready for pedestrian routing. These road types were added to lists below.

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

// Heuristics:
// For less pedestrian roads we add fine by setting smaller value of weight speed, and for more pedestrian roads we
// set greater values of weight speed. Algorithm picks roads with greater speed first,
// preferencing a more pedestrian roads over less pedestrian.
// As result of such heuristic road is not totally the shortest, but it avoids non pedestrian roads, which were
// not marked as "foot=no" in OSM.

HighwayFactors const kDefaultFactors = GetOneFactorsForBicycleAndPedestrianModel();

HighwaySpeeds const kAllSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city(weight, eta), out city(weight eta))}
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(SpeedKMpH(1.0, 5.0))},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(SpeedKMpH(1.0, 5.0))},
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(SpeedKMpH(3.0, 5.0))},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(SpeedKMpH(3.0, 5.0))},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(SpeedKMpH(3.0, 5.0))},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(SpeedKMpH(3.0, 5.0))},
    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(SpeedKMpH(4.0, 5.0))},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH(SpeedKMpH(4.0, 5.0))},
    {HighwayType::HighwayService, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(SpeedKMpH(4.5, 5.0))},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH(SpeedKMpH(4.0, 5.0))},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwayPath, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwayBridleway, InOutCitySpeedKMpH(SpeedKMpH(1.0, 5.0))},
    {HighwayType::HighwayCycleway, InOutCitySpeedKMpH(SpeedKMpH(4.0, 5.0))},
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH(SpeedKMpH(4.5, 5.0))},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwaySteps, InOutCitySpeedKMpH(SpeedKMpH(3.0))},
    {HighwayType::HighwayPedestrian, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwayFootway, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    /// @todo Set the same speed as a ferry for bicycle. At the same time, a car ferry has {10, 10}.
    {HighwayType::RouteFerry, InOutCitySpeedKMpH(SpeedKMpH(3.0, 20.0))},
};

SpeedKMpH constexpr kOffroadSpeed = {3.0 /* weight */, 3.0 /* eta */};

// Default (No bridleway, no cycleway).
HighwaySpeeds DefaultSpeeds()
{
  HighwaySpeeds res = kAllSpeeds;
  res.erase(HighwayType::HighwayBridleway);
  res.erase(HighwayType::HighwayCycleway);
  return res;
}

// Same as defaults except trunk and trunk link are not allowed.
HighwaySpeeds NoTrunk()
{
  HighwaySpeeds res = DefaultSpeeds();
  res.erase(HighwayType::HighwayTrunk);
  res.erase(HighwayType::HighwayTrunkLink);
  return res;
}

// Same as defaults except cycleway is allowed.
HighwaySpeeds AllowCycleway()
{
  HighwaySpeeds res = DefaultSpeeds();
  res[HighwayType::HighwayCycleway] = kAllSpeeds.at(HighwayType::HighwayCycleway);
  return res;
}

// Same as defaults except cycleway is allowed and trunk and trunk_link are not allowed.
HighwaySpeeds AllowCyclewayNoTrunk()
{
  HighwaySpeeds res = NoTrunk();
  res[HighwayType::HighwayCycleway] = kAllSpeeds.at(HighwayType::HighwayCycleway);
  return res;
}

// Belgium
// Trunk and trunk_link are not allowed
// Bridleway and cycleway are allowed
HighwaySpeeds BelgiumSpeeds()
{
  HighwaySpeeds res = AllowCyclewayNoTrunk();
  res[HighwayType::HighwayBridleway] = kAllSpeeds.at(HighwayType::HighwayBridleway);
  return res;
}

SurfaceFactors const kPedestrianSurface = {
  // {SurfaceType, {weightFactor, etaFactor}}
  {SurfaceType::PavedGood, {1.0, 1.0}},
  {SurfaceType::PavedBad, {1.0, 1.0}},
  {SurfaceType::UnpavedGood, {1.0, 1.0}},
  {SurfaceType::UnpavedBad, {0.8, 0.8}}
};

}  // namespace pedestrian_model

namespace routing
{
PedestrianModel::PedestrianModel(HighwaySpeeds const & speeds)
  : VehicleModel(speeds, pedestrian_model::kDefaultFactors, pedestrian_model::kPedestrianSurface)
{
  m_noFootType = classif().GetTypeByPath({"hwtag", "nofoot"});
  m_yesFootType = classif().GetTypeByPath({"hwtag", "yesfoot"});
}

SpeedKMpH PedestrianModel::GetOffroadSpeed() const
{
  return pedestrian_model::kOffroadSpeed;
}

VehicleModel::ResultT PedestrianModel::IsOneWay(uint32_t) const
{
  return ResultT::Unknown;
}

VehicleModel::ResultT PedestrianModel::GetRoadAvailability(uint32_t type) const
{
  if (type == m_yesFootType)
    return ResultT::Yes;
  else if (type == m_noFootType)
    return ResultT::No;
  return ResultT::Unknown;
}

SpeedKMpH PedestrianModel::GetSpeedForAvailable() const
{
  return m_maxModelSpeed.m_inCity;
}

// If one of feature types will be disabled for pedestrian, features of this type will be simplyfied
// in generator. Look FeatureBuilder1::IsRoad() for more details.
// static
PedestrianModel const & PedestrianModel::AllLimitsInstance()
{
  static PedestrianModel const instance(pedestrian_model::kAllSpeeds);
  return instance;
}

PedestrianModelFactory::PedestrianModelFactory(CountryParentNameGetterFn const & parentGetter)
  : VehicleModelFactory(parentGetter)
{
  using namespace pedestrian_model;
  // Names must be the same with country names from countries.txt

  m_models[""].reset(new PedestrianModel(DefaultSpeeds()));
  m_models["Australia"].reset(new PedestrianModel(kAllSpeeds));
  m_models["Austria"].reset(new PedestrianModel(NoTrunk()));
  m_models["Belarus"].reset(new PedestrianModel(AllowCycleway()));
  m_models["Belgium"].reset(new PedestrianModel(BelgiumSpeeds()));
  m_models["Brazil"].reset(new PedestrianModel(kAllSpeeds));
  m_models["Denmark"].reset(new PedestrianModel(AllowCyclewayNoTrunk()));
  m_models["France"].reset(new PedestrianModel(NoTrunk()));
  m_models["Finland"].reset(new PedestrianModel(AllowCycleway()));
  m_models["Germany"].reset(new PedestrianModel(DefaultSpeeds()));
  m_models["Hungary"].reset(new PedestrianModel(NoTrunk()));
  m_models["Iceland"].reset(new PedestrianModel(kAllSpeeds));
  m_models["Netherlands"].reset(new PedestrianModel(AllowCyclewayNoTrunk()));
  m_models["Norway"].reset(new PedestrianModel(kAllSpeeds));
  m_models["Oman"].reset(new PedestrianModel(kAllSpeeds));
  m_models["Poland"].reset(new PedestrianModel(NoTrunk()));
  m_models["Romania"].reset(new PedestrianModel(NoTrunk()));
  m_models["Russian Federation"].reset(new PedestrianModel(AllowCycleway()));
  m_models["Slovakia"].reset(new PedestrianModel(NoTrunk()));
  m_models["Spain"].reset(new PedestrianModel(NoTrunk()));
  m_models["Switzerland"].reset(new PedestrianModel(NoTrunk()));
  m_models["Turkey"].reset(new PedestrianModel(kAllSpeeds));
  m_models["Ukraine"].reset(new PedestrianModel(NoTrunk()));
  m_models["United Kingdom"].reset(new PedestrianModel(kAllSpeeds));
  m_models["United States of America"].reset(new PedestrianModel(kAllSpeeds));
}

}  // routing
