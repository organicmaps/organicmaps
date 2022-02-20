#include "routing_common/bicycle_model.hpp"

#include "indexer/classificator.hpp"

namespace bicycle_model
{
using namespace routing;

// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions
// Document contains proposals for some countries, but we assume that some kinds of roads are ready for bicycle routing,
// but not listed in tables in the document. For example, steps are not listed, paths, roads and services features also
// can be treated as ready for bicycle routing. These road types were added to lists below.

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

// Heuristics:
// For less bicycle roads we add fine by setting smaller value of weight speed, and for more bicycle roads we
// set greater values of weight speed. Algorithm picks roads with greater weight speed first,
// preferencing a more bicycle roads over less bicycle.
// As result of such heuristic road is not totally the shortest, but it avoids non bicycle roads, which were
// not marked as "hwtag=nobicycle" in OSM.

HighwayFactors const kDefaultFactors = GetOneFactorsForBicycleAndPedestrianModel();

HighwaySpeeds const kDefaultSpeeds = {
    // {highway class, InOutCitySpeedKMpH(in city(weight, eta), out city(weight eta))}
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(SpeedKMpH(3.0, 18.0))},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(SpeedKMpH(3.0, 18.0))},
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(SpeedKMpH(10.0, 18.0), SpeedKMpH(14.0, 18.0))},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(SpeedKMpH(10.0, 18.0), SpeedKMpH(14.0, 18.0))},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(SpeedKMpH(15.0, 18.0), SpeedKMpH(20.0, 18.0))},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(SpeedKMpH(15.0, 18.0), SpeedKMpH(20.0, 18.0))},
    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(SpeedKMpH(15.0, 18.0), SpeedKMpH(20.0, 18.0))},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH(SpeedKMpH(15.0, 18.0), SpeedKMpH(20.0, 18.0))},
    {HighwayType::HighwayService, InOutCitySpeedKMpH(SpeedKMpH(12.0, 18.0))},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(SpeedKMpH(12.0, 18.0))},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH(SpeedKMpH(10.0, 12.0))},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH(SpeedKMpH(8.0, 12.0))},
    {HighwayType::HighwayPath, InOutCitySpeedKMpH(SpeedKMpH(6.0, 12.0))},
    {HighwayType::HighwayCycleway, InOutCitySpeedKMpH(SpeedKMpH(30.0, 20.0))},
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH(SpeedKMpH(8.0, 10.0))},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(SpeedKMpH(7.0, 8.0))},

    {HighwayType::RouteFerry, InOutCitySpeedKMpH(SpeedKMpH(3.0, 20.0))},

    // Steps have obvious inconvenience of a bike in hands.
    {HighwayType::HighwaySteps, InOutCitySpeedKMpH(SpeedKMpH(1.0, 1.0))},

    // Set minimun default bicycle speeds for the next disputable roads.
    {HighwayType::HighwayPedestrian, InOutCitySpeedKMpH(SpeedKMpH(2.0))},
    {HighwayType::HighwayFootway, InOutCitySpeedKMpH(SpeedKMpH(2.0))},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH(SpeedKMpH(2.0))},
    {HighwayType::HighwayBridleway, InOutCitySpeedKMpH(SpeedKMpH(2.0))},
};

// Normal bicycle speeds for disputable roads.
HighwaySpeeds const kAdditionalSpeeds = {
    {HighwayType::HighwayPedestrian, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwayFootway, InOutCitySpeedKMpH(SpeedKMpH(7.0, 5.0))},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH(SpeedKMpH(7.0))},
    {HighwayType::HighwayBridleway, InOutCitySpeedKMpH(SpeedKMpH(4.0, 12.0))},
};

SpeedKMpH constexpr kOffroadSpeed = {3.0 /* weight */, 3.0 /* eta */};

// Same as defaults except trunk and trunk_link are not allowed
HighwaySpeeds NoTrunk()
{
  HighwaySpeeds res = kDefaultSpeeds;
  res.erase(HighwayType::HighwayTrunk);
  res.erase(HighwayType::HighwayTrunkLink);
  return res;
}

// Same as defaults except pedestrian is allowed
HighwaySpeeds PedestrianAllowed()
{
  HighwaySpeeds res = kDefaultSpeeds;
  res[HighwayType::HighwayPedestrian] = kAdditionalSpeeds.at(HighwayType::HighwayPedestrian);
  return res;
}

// Same as defaults except bridleway is allowed
HighwaySpeeds BridlewayAllowed()
{
  HighwaySpeeds res = kDefaultSpeeds;
  res[HighwayType::HighwayBridleway] = kAdditionalSpeeds.at(HighwayType::HighwayBridleway);
  return res;
}

// Same as defaults except pedestrian and footway are allowed
HighwaySpeeds PedestrianFootwayAllowed()
{
  HighwaySpeeds res = kDefaultSpeeds;
  res[HighwayType::HighwayPedestrian] = kAdditionalSpeeds.at(HighwayType::HighwayPedestrian);
  res[HighwayType::HighwayFootway] = kAdditionalSpeeds.at(HighwayType::HighwayFootway);
  return res;
}

HighwaySpeeds AllAllowed()
{
  HighwaySpeeds res = kDefaultSpeeds;
  for (auto const & e : kAdditionalSpeeds)
    res[e.first] = e.second;
  return res;
}

// Austria (No trunk, no path)
HighwaySpeeds AustriaSpeeds()
{
  HighwaySpeeds res = NoTrunk();
  res.erase(HighwayType::HighwayPath);
  return res;
}

// Belgium (No trunk, pedestrian is allowed).
HighwaySpeeds BelgiumSpeeds()
{
  HighwaySpeeds res = NoTrunk();
  res[HighwayType::HighwayPedestrian] = kAdditionalSpeeds.at(HighwayType::HighwayPedestrian);
  return res;
}

// Brazil (Bridleway and footway are allowed)
HighwaySpeeds BrazilSpeeds()
{
  HighwaySpeeds res = kDefaultSpeeds;
  res[HighwayType::HighwayBridleway] = kAdditionalSpeeds.at(HighwayType::HighwayBridleway);
  res[HighwayType::HighwayFootway] = kAdditionalSpeeds.at(HighwayType::HighwayFootway);
  return res;
}

// France, same as Belgium (No trunk, pedestrian is allowed);
HighwaySpeeds FranceSpeeds() { return BelgiumSpeeds(); }

// Ukraine (No trunk, footway and pedestrian are allowed).
HighwaySpeeds UkraineSpeeds()
{
  HighwaySpeeds res = NoTrunk();
  res[HighwayType::HighwayPedestrian] = kAdditionalSpeeds.at(HighwayType::HighwayPedestrian);
  res[HighwayType::HighwayFootway] = kAdditionalSpeeds.at(HighwayType::HighwayFootway);
  return res;
}

// United States of America (Bridleway and pedestrian are allowed)
HighwaySpeeds USASpeeds()
{
  HighwaySpeeds res = kDefaultSpeeds;
  res[HighwayType::HighwayBridleway] = kAdditionalSpeeds.at(HighwayType::HighwayBridleway);
  res[HighwayType::HighwayPedestrian] = kAdditionalSpeeds.at(HighwayType::HighwayPedestrian);
  return res;
}

SurfaceFactors const kBicycleSurface = {
  // {SurfaceType, {weightFactor, etaFactor}}
  {SurfaceType::PavedGood, {1.0, 1.0}},
  {SurfaceType::PavedBad, {0.8, 0.8}},
  {SurfaceType::UnpavedGood, {1.0, 1.0}},
  {SurfaceType::UnpavedBad, {0.3, 0.3}}
};

}  // namespace bicycle_model

namespace routing
{

BicycleModel::BicycleModel(HighwaySpeeds const & speeds)
  : VehicleModel(speeds, bicycle_model::kDefaultFactors, bicycle_model::kBicycleSurface)
{
  auto const & cl = classif();
  m_yesBicycleType = cl.GetTypeByPath({"hwtag", "yesbicycle"});
  m_noBicycleType = cl.GetTypeByPath({"hwtag", "nobicycle"});
  m_bidirBicycleType = cl.GetTypeByPath({"hwtag", "bidir_bicycle"});
  m_onedirBicycleType = cl.GetTypeByPath({"hwtag", "onedir_bicycle"});
}

VehicleModel::ResultT BicycleModel::IsOneWay(uint32_t type) const
{
  if (type == m_onedirBicycleType)
    return ResultT::Yes;
  else if (type == m_bidirBicycleType)
    return ResultT::No;
  return ResultT::Unknown;
}

VehicleModel::ResultT BicycleModel::GetRoadAvailability(uint32_t type) const
{
  if (type == m_yesBicycleType)
    return ResultT::Yes;
  else if (type == m_noBicycleType)
    return ResultT::No;
  return ResultT::Unknown;
}

SpeedKMpH BicycleModel::GetSpeedForAvailable() const
{
  return m_maxModelSpeed.m_inCity;
}

SpeedKMpH BicycleModel::GetOffroadSpeed() const
{
  return bicycle_model::kOffroadSpeed;
}

// If one of feature types will be disabled for bicycles, features of this type will be simplified
// in generator. Look FeatureBuilder1::IsRoad() for more details.
// static
BicycleModel const & BicycleModel::AllLimitsInstance()
{
  static BicycleModel const instance(bicycle_model::AllAllowed());
  return instance;
}

BicycleModelFactory::BicycleModelFactory(CountryParentNameGetterFn const & parentGetter)
  : VehicleModelFactory(parentGetter)
{
  using namespace bicycle_model;
  // Names must be the same with country names from countries.txt

  m_models[""].reset(new BicycleModel(kDefaultSpeeds));
  m_models["Australia"].reset(new BicycleModel(AllAllowed()));
  m_models["Austria"].reset(new BicycleModel(AustriaSpeeds()));
  m_models["Belarus"].reset(new BicycleModel(PedestrianFootwayAllowed()));
  m_models["Belgium"].reset(new BicycleModel(BelgiumSpeeds()));
  m_models["Brazil"].reset(new BicycleModel(BrazilSpeeds()));
  m_models["Denmark"].reset(new BicycleModel(NoTrunk()));
  m_models["France"].reset(new BicycleModel(FranceSpeeds()));
  m_models["Finland"].reset(new BicycleModel(PedestrianAllowed()));
  m_models["Germany"].reset(new BicycleModel(kDefaultSpeeds));
  m_models["Hungary"].reset(new BicycleModel(NoTrunk()));
  m_models["Iceland"].reset(new BicycleModel(AllAllowed()));
  m_models["Netherlands"].reset(new BicycleModel(NoTrunk()));
  m_models["Norway"].reset(new BicycleModel(AllAllowed()));
  m_models["Oman"].reset(new BicycleModel(BridlewayAllowed()));
  m_models["Poland"].reset(new BicycleModel(NoTrunk()));
  m_models["Romania"].reset(new BicycleModel(NoTrunk()));

  // Russian Federation
  // Footway and pedestrian are allowed
  // Note. Despite the fact that according to
  // https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions
  // passing through service and living_street with a bicycle is prohibited
  // it's allowed according to Russian traffic rules.
  m_models["Russian Federation"].reset(new BicycleModel(PedestrianFootwayAllowed()));

  m_models["Slovakia"].reset(new BicycleModel(NoTrunk()));
  m_models["Spain"].reset(new BicycleModel(PedestrianAllowed()));
  m_models["Switzerland"].reset(new BicycleModel(NoTrunk()));
  m_models["Turkey"].reset(new BicycleModel(kDefaultSpeeds));
  m_models["Ukraine"].reset(new BicycleModel(UkraineSpeeds()));
  m_models["United Kingdom"].reset(new BicycleModel(BridlewayAllowed()));
  m_models["United States of America"].reset(new BicycleModel(USASpeeds()));
}

}  // routing
