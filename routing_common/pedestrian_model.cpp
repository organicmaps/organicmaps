#include "routing_common/pedestrian_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

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

HighwayBasedFactors const kDefaultFactors = GetOneFactorsForBicycleAndPedestrianModel();

HighwayBasedSpeeds const kDefaultSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city(weight, eta), out city(weight eta))}
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(SpeedKMpH(1.0, 5.0))},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(SpeedKMpH(1.0, 5.0))},
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(SpeedKMpH(2.0, 5.0))},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(SpeedKMpH(2.0, 5.0))},
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
    /// @todo A car ferry has {10, 10}. Weight = 3 is 60% from reasonable 5 max speed.
    {HighwayType::RouteFerry, InOutCitySpeedKMpH(SpeedKMpH(3.0, 20.0))},
};

// https://github.com/organicmaps/organicmaps/issues/2492
// 3 kmph (was before) is a big default offroad speed, almost as normal walking speed.
SpeedKMpH constexpr kSpeedOffroadKMpH = {0.5 /* weight */, 3.0 /* eta */};

// Default, no bridleway and cycleway
VehicleModel::LimitsInitList const kDefaultOptions = {
    // {HighwayType, passThroughAllowed}
    {HighwayType::HighwayTrunk, true},
    {HighwayType::HighwayTrunkLink, true},
    {HighwayType::HighwayPrimary, true},
    {HighwayType::HighwayPrimaryLink, true},
    {HighwayType::HighwaySecondary, true},
    {HighwayType::HighwaySecondaryLink, true},
    {HighwayType::HighwayTertiary, true},
    {HighwayType::HighwayTertiaryLink, true},
    {HighwayType::HighwayService, true},
    {HighwayType::HighwayUnclassified, true},
    {HighwayType::HighwayRoad, true},
    {HighwayType::HighwayTrack, true},
    {HighwayType::HighwayPath, true},
    // HighwayBridleway, HighwayCycleway are missing
    {HighwayType::HighwayResidential, true},
    {HighwayType::HighwayLivingStreet, true},
    {HighwayType::HighwaySteps, true},
    {HighwayType::HighwayPedestrian, true},
    {HighwayType::HighwayFootway, true},
    {HighwayType::ManMadePier, true},
    {HighwayType::RouteFerry, true}
};

// Same as defaults except bridleway and cycleway are allowed.
VehicleModel::LimitsInitList AllAllowed()
{
  auto res = kDefaultOptions;
  res.push_back({HighwayType::HighwayBridleway, true});
  res.push_back({HighwayType::HighwayCycleway, true});
  return res;
}

// Same as defaults except trunk and trunk link are not allowed.
VehicleModel::LimitsInitList NoTrunk()
{
  VehicleModel::LimitsInitList res;
  res.reserve(kDefaultOptions.size() - 2);
  for (auto const & e : kDefaultOptions)
  {
    if (e.m_type != HighwayType::HighwayTrunk && e.m_type != HighwayType::HighwayTrunkLink)
      res.push_back(e);
  }
  return res;
}

VehicleModel::LimitsInitList YesCycleway(VehicleModel::LimitsInitList res = kDefaultOptions)
{
  res.push_back({HighwayType::HighwayCycleway, true});
  return res;
}

VehicleModel::LimitsInitList YesBridleway(VehicleModel::LimitsInitList res = kDefaultOptions)
{
  res.push_back({HighwayType::HighwayBridleway, true});
  return res;
}

VehicleModel::SurfaceInitList const kPedestrianSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {1.0, 1.0}},
  {{"psurface", "unpaved_good"}, {1.0, 1.0}},
  {{"psurface", "unpaved_bad"}, {0.8, 0.8}}
};
}  // namespace pedestrian_model

namespace routing
{
PedestrianModel::PedestrianModel() : PedestrianModel(pedestrian_model::kDefaultOptions)
{
}

PedestrianModel::PedestrianModel(VehicleModel::LimitsInitList const & speedLimits)
  : VehicleModel(classif(), speedLimits, pedestrian_model::kPedestrianSurface,
                {pedestrian_model::kDefaultSpeeds, pedestrian_model::kDefaultFactors})
{
  using namespace pedestrian_model;

  // No bridleway and cycleway in default.
  ASSERT_EQUAL(kDefaultOptions.size(), kDefaultSpeeds.size() - 2, ());

  std::vector<std::string> hwtagYesFoot = {"hwtag", "yesfoot"};
  auto const & cl = classif();

  m_noType = cl.GetTypeByPath({ "hwtag", "nofoot" });
  m_yesType = cl.GetTypeByPath(hwtagYesFoot);

  AddAdditionalRoadTypes(cl, {{ std::move(hwtagYesFoot), kDefaultSpeeds.Get(HighwayType::HighwayFootway) }});

  // Update max pedestrian speed with possible ferry transfer. See EdgeEstimator::CalcHeuristic.
  SpeedKMpH constexpr kMaxPedestrianSpeedKMpH(25.0);
  CHECK_LESS(m_maxModelSpeed, kMaxPedestrianSpeedKMpH, ());
  m_maxModelSpeed = kMaxPedestrianSpeedKMpH;
}

SpeedKMpH PedestrianModel::GetTypeSpeed(feature::TypesHolder const & types, SpeedParams const & speedParams) const
{
  return GetTypeSpeedImpl(types, speedParams, false /* isCar */);
}

SpeedKMpH const & PedestrianModel::GetOffroadSpeed() const { return pedestrian_model::kSpeedOffroadKMpH; }

// If one of feature types will be disabled for pedestrian, features of this type will be simplyfied
// in generator. Look FeatureBuilder1::IsRoad() for more details.
// static
PedestrianModel const & PedestrianModel::AllLimitsInstance()
{
  static PedestrianModel const instance(pedestrian_model::AllAllowed());
  return instance;
}

PedestrianModelFactory::PedestrianModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  using namespace pedestrian_model;
  using std::make_shared;

  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<PedestrianModel>(kDefaultOptions);

  m_models["Australia"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Austria"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["Belarus"] = make_shared<PedestrianModel>(YesCycleway());
  m_models["Belgium"] = make_shared<PedestrianModel>(YesCycleway(YesBridleway(NoTrunk())));
  m_models["Brazil"] = make_shared<PedestrianModel>(YesBridleway());
  m_models["Denmark"] = make_shared<PedestrianModel>(YesCycleway(NoTrunk()));
  m_models["France"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["Finland"] = make_shared<PedestrianModel>(YesCycleway());
  m_models["Greece"] = make_shared<PedestrianModel>(YesCycleway(YesBridleway(NoTrunk())));
  m_models["Hungary"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["Iceland"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Ireland"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Netherlands"] = make_shared<PedestrianModel>(YesCycleway(NoTrunk()));
  m_models["Norway"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Oman"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Philippines"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Poland"] = make_shared<PedestrianModel>(YesBridleway(NoTrunk()));
  m_models["Romania"] = make_shared<PedestrianModel>(YesBridleway());
  m_models["Russian Federation"] = make_shared<PedestrianModel>(YesCycleway());
  m_models["Slovakia"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["Spain"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["Sweden"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Switzerland"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["Turkey"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["Ukraine"] = make_shared<PedestrianModel>(NoTrunk());
  m_models["United Kingdom"] = make_shared<PedestrianModel>(AllAllowed());
  m_models["United States of America"] = make_shared<PedestrianModel>(AllAllowed());
}
}  // routing
