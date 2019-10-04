#include "routing_common/pedestrian_model.hpp"

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
    {HighwayType::ManMadePier, InOutCitySpeedKMpH(SpeedKMpH(7.0))},
    {HighwayType::RouteFerry, InOutCitySpeedKMpH(SpeedKMpH(1.0, 20.0))},
};

SpeedKMpH constexpr kSpeedOffroadKMpH = {3.0 /* weight */, 3.0 /* eta */};

// Default
VehicleModel::LimitsInitList const kPedestrianOptionsDefault = {
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
    {{"highway", "residential"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "steps"}, true},
    {{"highway", "pedestrian"}, true},
    {{"highway", "footway"}, true}};

// All options available.
VehicleModel::LimitsInitList const kPedestrianOptionsAll = {
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

// Same as defaults except trunk and trunk link are not allowed.
VehicleModel::LimitsInitList const kPedestrianOptionsNoTrunk = {
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

// Same as defaults except cycleway is allowed.
VehicleModel::LimitsInitList const kPedestrianOptionsCyclewayAllowed = {
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

// Same as defaults except cycleway is allowed and trunk and trunk_link are not allowed.
VehicleModel::LimitsInitList const kPedestrianOptionsCyclewayAllowedNoTrunk = {
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
VehicleModel::LimitsInitList const kPedestrianOptionsAustralia = kPedestrianOptionsAll;

// Austria
VehicleModel::LimitsInitList const kPedestrianOptionsAustria = kPedestrianOptionsNoTrunk;

// Belarus
VehicleModel::LimitsInitList const kPedestrianOptionsBelarus = kPedestrianOptionsCyclewayAllowed;

// Belgium
VehicleModel::LimitsInitList const kPedestrianOptionsBelgium = {
    // Trunk and trunk_link are not allowed
    // Bridleway and cycleway are allowed
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

// Brazil
VehicleModel::LimitsInitList const kPedestrianOptionsBrazil = kPedestrianOptionsAll;

// Denmark
VehicleModel::LimitsInitList const kPedestrianOptionsDenmark = kPedestrianOptionsCyclewayAllowedNoTrunk;

// France
VehicleModel::LimitsInitList const kPedestrianOptionsFrance = kPedestrianOptionsNoTrunk;

// Finland
VehicleModel::LimitsInitList const kPedestrianOptionsFinland = kPedestrianOptionsCyclewayAllowed;

// Germany
VehicleModel::LimitsInitList const kPedestrianOptionsGermany = kPedestrianOptionsDefault;

// Hungary
VehicleModel::LimitsInitList const kPedestrianOptionsHungary = kPedestrianOptionsNoTrunk;

// Iceland
VehicleModel::LimitsInitList const kPedestrianOptionsIceland = kPedestrianOptionsAll;

// Netherlands
VehicleModel::LimitsInitList const kPedestrianOptionsNetherlands = kPedestrianOptionsCyclewayAllowedNoTrunk;

// Norway
VehicleModel::LimitsInitList const kPedestrianOptionsNorway = kPedestrianOptionsAll;

// Oman
VehicleModel::LimitsInitList const kPedestrianOptionsOman = kPedestrianOptionsAll;

// Poland
VehicleModel::LimitsInitList const kPedestrianOptionsPoland = kPedestrianOptionsNoTrunk;

// Romania
VehicleModel::LimitsInitList const kPedestrianOptionsRomania = kPedestrianOptionsNoTrunk;

// Russian Federation
VehicleModel::LimitsInitList const kPedestrianOptionsRussia = kPedestrianOptionsCyclewayAllowed;

// Slovakia
VehicleModel::LimitsInitList const kPedestrianOptionsSlovakia = kPedestrianOptionsNoTrunk;

// Spain
VehicleModel::LimitsInitList const kPedestrianOptionsSpain = kPedestrianOptionsNoTrunk;

// Switzerland
VehicleModel::LimitsInitList const kPedestrianOptionsSwitzerland = kPedestrianOptionsNoTrunk;

// Turkey
VehicleModel::LimitsInitList const kPedestrianOptionsTurkey = kPedestrianOptionsAll;

// Ukraine
VehicleModel::LimitsInitList const kPedestrianOptionsUkraine = kPedestrianOptionsNoTrunk;

// United Kingdom
VehicleModel::LimitsInitList const kPedestrianOptionsUK = kPedestrianOptionsAll;

// United States of America
VehicleModel::LimitsInitList const kPedestrianOptionsUS = kPedestrianOptionsAll;

VehicleModel::SurfaceInitList const kPedestrianSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {1.0, 1.0}},
  {{"psurface", "unpaved_good"}, {1.0, 1.0}},
  {{"psurface", "unpaved_bad"}, {0.8, 0.8}}
};
}  // namespace

namespace routing
{
PedestrianModel::PedestrianModel()
  : VehicleModel(classif(), kPedestrianOptionsDefault, kPedestrianSurface,
                 {kDefaultSpeeds, kDefaultFactors})
{
  Init();
}

PedestrianModel::PedestrianModel(VehicleModel::LimitsInitList const & speedLimits)
  : VehicleModel(classif(), speedLimits, kPedestrianSurface, {kDefaultSpeeds, kDefaultFactors})
{
  Init();
}

SpeedKMpH PedestrianModel::GetSpeed(FeatureType & f, SpeedParams const & speedParams) const
{
  return VehicleModel::GetSpeedWihtoutMaxspeed(f, speedParams);
}

SpeedKMpH const & PedestrianModel::GetOffroadSpeed() const { return kSpeedOffroadKMpH; }

void PedestrianModel::Init()
{
  initializer_list<char const *> hwtagYesFoot = {"hwtag", "yesfoot"};

  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });
  m_yesFootType = classif().GetTypeByPath(hwtagYesFoot);

  vector<AdditionalRoadTags> const additionalTags = {
      {hwtagYesFoot, m_maxModelSpeed},
      {{"route", "ferry"}, kDefaultSpeeds.at(HighwayType::RouteFerry)},
      {{"man_made", "pier"}, kDefaultSpeeds.at(HighwayType::ManMadePier)}};

  SetAdditionalRoadTypes(classif(), additionalTags);
}

VehicleModelInterface::RoadAvailability PedestrianModel::GetRoadAvailability(feature::TypesHolder const & types) const
{
  if (types.Has(m_yesFootType))
    return RoadAvailability::Available;

  if (types.Has(m_noFootType))
    return RoadAvailability::NotAvailable;

  return RoadAvailability::Unknown;
}

// If one of feature types will be disabled for pedestrian, features of this type will be simplyfied
// in generator. Look FeatureBuilder1::IsRoad() for more details.
// static
PedestrianModel const & PedestrianModel::AllLimitsInstance()
{
  static PedestrianModel const instance(kPedestrianOptionsAll);
  return instance;
}

PedestrianModelFactory::PedestrianModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<PedestrianModel>(kPedestrianOptionsDefault);
  m_models["Australia"] = make_shared<PedestrianModel>(kPedestrianOptionsAustralia);
  m_models["Austria"] = make_shared<PedestrianModel>(kPedestrianOptionsAustria);
  m_models["Belarus"] = make_shared<PedestrianModel>(kPedestrianOptionsBelarus);
  m_models["Belgium"] = make_shared<PedestrianModel>(kPedestrianOptionsBelgium);
  m_models["Brazil"] = make_shared<PedestrianModel>(kPedestrianOptionsBrazil);
  m_models["Denmark"] = make_shared<PedestrianModel>(kPedestrianOptionsDenmark);
  m_models["France"] = make_shared<PedestrianModel>(kPedestrianOptionsFrance);
  m_models["Finland"] = make_shared<PedestrianModel>(kPedestrianOptionsFinland);
  m_models["Germany"] = make_shared<PedestrianModel>(kPedestrianOptionsGermany);
  m_models["Hungary"] = make_shared<PedestrianModel>(kPedestrianOptionsHungary);
  m_models["Iceland"] = make_shared<PedestrianModel>(kPedestrianOptionsIceland);
  m_models["Netherlands"] = make_shared<PedestrianModel>(kPedestrianOptionsNetherlands);
  m_models["Norway"] = make_shared<PedestrianModel>(kPedestrianOptionsNorway);
  m_models["Oman"] = make_shared<PedestrianModel>(kPedestrianOptionsOman);
  m_models["Poland"] = make_shared<PedestrianModel>(kPedestrianOptionsPoland);
  m_models["Romania"] = make_shared<PedestrianModel>(kPedestrianOptionsRomania);
  m_models["Russian Federation"] = make_shared<PedestrianModel>(kPedestrianOptionsRussia);
  m_models["Slovakia"] = make_shared<PedestrianModel>(kPedestrianOptionsSlovakia);
  m_models["Spain"] = make_shared<PedestrianModel>(kPedestrianOptionsSpain);
  m_models["Switzerland"] = make_shared<PedestrianModel>(kPedestrianOptionsSwitzerland);
  m_models["Turkey"] = make_shared<PedestrianModel>(kPedestrianOptionsTurkey);
  m_models["Ukraine"] = make_shared<PedestrianModel>(kPedestrianOptionsUkraine);
  m_models["United Kingdom"] = make_shared<PedestrianModel>(kPedestrianOptionsUK);
  m_models["United States of America"] = make_shared<PedestrianModel>(kPedestrianOptionsUS);
}
}  // routing
