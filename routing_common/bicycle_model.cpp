#include "routing_common/bicycle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/logging.hpp"

using namespace routing;
using namespace std;

namespace
{
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

HighwayBasedFactors const kDefaultFactors = GetOneFactorsForBicycleAndPedestrianModel();

HighwayBasedSpeeds const kDefaultSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city(weight, eta), out city(weight eta))}
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
    {HighwayType::HighwayBridleway, InOutCitySpeedKMpH(SpeedKMpH(4.0, 12.0))},
    {HighwayType::HighwayCycleway, InOutCitySpeedKMpH(SpeedKMpH(30.0, 20.0))},
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH(SpeedKMpH(8.0, 10.0))},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(SpeedKMpH(7.0, 8.0))},
    {HighwayType::HighwaySteps, InOutCitySpeedKMpH(SpeedKMpH(1.0, 5.0))},
    {HighwayType::HighwayPedestrian, InOutCitySpeedKMpH(SpeedKMpH(5.0))},
    {HighwayType::HighwayFootway, InOutCitySpeedKMpH(SpeedKMpH(7.0, 5.0))},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH(SpeedKMpH(7.0))},
    {HighwayType::RouteFerry, InOutCitySpeedKMpH(SpeedKMpH(3.0, 20.0))},
};

SpeedKMpH constexpr kSpeedOffroadKMpH = {3.0 /* weight */, 3.0 /* eta */};

// Default
VehicleModel::LimitsInitList const kBicycleOptionsDefault = {
    // {{roadType, roadType} passThroughAllowed}
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true}};

// All options available.
VehicleModel::LimitsInitList const kBicycleOptionsAll = {
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "bridleway"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true},
    {{"highway", "footway"}, true}};

// Same as defaults except trunk and trunk_link are not allowed
VehicleModel::LimitsInitList const kBicycleOptionsNoTrunk = {
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true}};

// Same as defaults except pedestrian is allowed
VehicleModel::LimitsInitList const kBicycleOptionsPedestrianAllowed = {
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true}};

// Same as defaults except bridleway is allowed
VehicleModel::LimitsInitList const kBicycleOptionsBridlewayAllowed = {
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "bridleway"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true}};

// Same as defaults except pedestrian and footway are allowed
VehicleModel::LimitsInitList const kBicycleOptionsPedestrianFootwayAllowed = {
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true},
    {{"highway", "footway"}, true}};

// Australia
VehicleModel::LimitsInitList const kBicycleOptionsAustralia = kBicycleOptionsAll;

// Austria
VehicleModel::LimitsInitList const kBicycleOptionsAustria = {
    // No trunk, trunk_link, path
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true}};

// Belarus
VehicleModel::LimitsInitList const kBicycleOptionsBelarus = kBicycleOptionsPedestrianFootwayAllowed;

// Belgium
VehicleModel::LimitsInitList const kBicycleOptionsBelgium = {
    // No trunk, trunk_link
    // Pedestrian is allowed
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true}};

// Brazil
VehicleModel::LimitsInitList const kBicycleOptionsBrazil = {
    // Bridleway and fotway are allowed
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "bridleway"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "footway"}, true}};

// Denmark
VehicleModel::LimitsInitList const kBicycleOptionsDenmark = kBicycleOptionsNoTrunk;

// France
VehicleModel::LimitsInitList const kBicycleOptionsFrance = {
    // No trunk, trunk_link
    // Pedestrian is allowed
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true}};

// Finland
VehicleModel::LimitsInitList const kBicycleOptionsFinland = kBicycleOptionsPedestrianAllowed;

// Germany
VehicleModel::LimitsInitList const kBicycleOptionsGermany = kBicycleOptionsDefault;

// Hungary
VehicleModel::LimitsInitList const kBicycleOptionsHungary = kBicycleOptionsNoTrunk;

// Iceland
VehicleModel::LimitsInitList const kBicycleOptionsIceland = kBicycleOptionsAll;

// Netherlands
VehicleModel::LimitsInitList const kBicycleOptionsNetherlands = kBicycleOptionsNoTrunk;

// Norway
VehicleModel::LimitsInitList const kBicycleOptionsNorway = kBicycleOptionsAll;

// Oman
VehicleModel::LimitsInitList const kBicycleOptionsOman = kBicycleOptionsBridlewayAllowed;

// Poland
VehicleModel::LimitsInitList const kBicycleOptionsPoland = kBicycleOptionsNoTrunk;

// Romania
VehicleModel::LimitsInitList const kBicycleOptionsRomania = kBicycleOptionsNoTrunk;

// Russian Federation
// Footway and pedestrian are allowed
// Note. Despite the fact that according to
// https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions
// passing through service and living_street with a bicycle is prohibited
// it's allowed according to Russian traffic rules.
VehicleModel::LimitsInitList const kBicycleOptionsRussia = kBicycleOptionsPedestrianFootwayAllowed;

// Slovakia
VehicleModel::LimitsInitList const kBicycleOptionsSlovakia = kBicycleOptionsNoTrunk;

// Spain
VehicleModel::LimitsInitList const kBicycleOptionsSpain = kBicycleOptionsPedestrianAllowed;

// Switzerland
VehicleModel::LimitsInitList const kBicycleOptionsSwitzerland = kBicycleOptionsNoTrunk;

// Turkey
VehicleModel::LimitsInitList const kBicycleOptionsTurkey = kBicycleOptionsDefault;

// Ukraine
VehicleModel::LimitsInitList const kBicycleOptionsUkraine = {
    // No trunk
    // Footway and pedestrian are allowed
    // No pass through living_street and service
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, false},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, false},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true},
    {{"highway", "footway"}, true}};

// United Kingdom
VehicleModel::LimitsInitList const kBicycleOptionsUK = kBicycleOptionsBridlewayAllowed;

// United States of America
VehicleModel::LimitsInitList const kBicycleOptionsUS = {
    // Bridleway and pedesprian are allowed
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "service"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true},
    {{"highway", "path"}, true},
    {{"highway", "bridleway"}, true},
    {{"highway", "cycleway"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true}};

VehicleModel::SurfaceInitList const kBicycleSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {0.8, 0.8}},
  {{"psurface", "unpaved_good"}, {1.0, 1.0}},
  {{"psurface", "unpaved_bad"}, {0.3, 0.3}}
};
}  // namespace

namespace routing
{
BicycleModel::BicycleModel()
  : VehicleModel(classif(), kBicycleOptionsDefault, kBicycleSurface,
                 {kDefaultSpeeds, kDefaultFactors})
{
  Init();
}

BicycleModel::BicycleModel(VehicleModel::LimitsInitList const & speedLimits)
  : VehicleModel(classif(), speedLimits, kBicycleSurface, {kDefaultSpeeds, kDefaultFactors})
{
  Init();
}

void BicycleModel::Init()
{
  initializer_list<char const *> hwtagYesBicycle = {"hwtag", "yesbicycle"};

  m_noBicycleType = classif().GetTypeByPath({"hwtag", "nobicycle"});
  m_yesBicycleType = classif().GetTypeByPath(hwtagYesBicycle);
  m_bidirBicycleType = classif().GetTypeByPath({"hwtag", "bidir_bicycle"});
  m_onedirBicycleType = classif().GetTypeByPath({"hwtag", "onedir_bicycle"});
  vector<AdditionalRoadTags> const additionalTags = {
      {hwtagYesBicycle, m_maxModelSpeed},
      {{"route", "ferry"}, kDefaultSpeeds.at(HighwayType::RouteFerry)},
      {{"man_made", "pier"}, kDefaultSpeeds.at(HighwayType::ManMadePier)}};

  SetAdditionalRoadTypes(classif(), additionalTags);
}

VehicleModelInterface::RoadAvailability BicycleModel::GetRoadAvailability(feature::TypesHolder const & types) const
{
  if (types.Has(m_yesBicycleType))
    return RoadAvailability::Available;

  if (types.Has(m_noBicycleType))
    return RoadAvailability::NotAvailable;

  return RoadAvailability::Unknown;
}

bool BicycleModel::IsBicycleBidir(feature::TypesHolder const & types) const
{
  return types.Has(m_bidirBicycleType);
}

bool BicycleModel::IsBicycleOnedir(feature::TypesHolder const & types) const
{
  return types.Has(m_onedirBicycleType);
}

SpeedKMpH BicycleModel::GetSpeed(FeatureType & f, SpeedParams const & speedParams) const
{
  return VehicleModel::GetSpeedWihtoutMaxspeed(f, speedParams);
}

bool BicycleModel::IsOneWay(FeatureType & f) const
{
  feature::TypesHolder const types(f);

  if (IsBicycleOnedir(types))
    return true;

  if (IsBicycleBidir(types))
    return false;

  return VehicleModel::IsOneWay(f);
}

SpeedKMpH const & BicycleModel::GetOffroadSpeed() const { return kSpeedOffroadKMpH; }

// If one of feature types will be disabled for bicycles, features of this type will be simplified
// in generator. Look FeatureBuilder1::IsRoad() for more details.
// static
BicycleModel const & BicycleModel::AllLimitsInstance()
{
  static BicycleModel const instance(kBicycleOptionsAll);
  return instance;
}

BicycleModelFactory::BicycleModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<BicycleModel>(kBicycleOptionsDefault);
  m_models["Australia"] = make_shared<BicycleModel>(kBicycleOptionsAustralia);
  m_models["Austria"] = make_shared<BicycleModel>(kBicycleOptionsAustria);
  m_models["Belarus"] = make_shared<BicycleModel>(kBicycleOptionsBelarus);
  m_models["Belgium"] = make_shared<BicycleModel>(kBicycleOptionsBelgium);
  m_models["Brazil"] = make_shared<BicycleModel>(kBicycleOptionsBrazil);
  m_models["Denmark"] = make_shared<BicycleModel>(kBicycleOptionsDenmark);
  m_models["France"] = make_shared<BicycleModel>(kBicycleOptionsFrance);
  m_models["Finland"] = make_shared<BicycleModel>(kBicycleOptionsFinland);
  m_models["Germany"] = make_shared<BicycleModel>(kBicycleOptionsGermany);
  m_models["Hungary"] = make_shared<BicycleModel>(kBicycleOptionsHungary);
  m_models["Iceland"] = make_shared<BicycleModel>(kBicycleOptionsIceland);
  m_models["Netherlands"] = make_shared<BicycleModel>(kBicycleOptionsNetherlands);
  m_models["Norway"] = make_shared<BicycleModel>(kBicycleOptionsNorway);
  m_models["Oman"] = make_shared<BicycleModel>(kBicycleOptionsOman);
  m_models["Poland"] = make_shared<BicycleModel>(kBicycleOptionsPoland);
  m_models["Romania"] = make_shared<BicycleModel>(kBicycleOptionsRomania);
  m_models["Russian Federation"] = make_shared<BicycleModel>(kBicycleOptionsRussia);
  m_models["Slovakia"] = make_shared<BicycleModel>(kBicycleOptionsSlovakia);
  m_models["Spain"] = make_shared<BicycleModel>(kBicycleOptionsSpain);
  m_models["Switzerland"] = make_shared<BicycleModel>(kBicycleOptionsSwitzerland);
  m_models["Turkey"] = make_shared<BicycleModel>(kBicycleOptionsTurkey);
  m_models["Ukraine"] = make_shared<BicycleModel>(kBicycleOptionsUkraine);
  m_models["United Kingdom"] = make_shared<BicycleModel>(kBicycleOptionsUK);
  m_models["United States of America"] = make_shared<BicycleModel>(kBicycleOptionsUS);
}
}  // routing
