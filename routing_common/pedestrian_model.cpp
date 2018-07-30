#include "routing_common/pedestrian_model.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/logging.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

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
// For less pedestrian roads we add fine by setting smaller value of speed, and for more pedestrian roads we
// set greater values of speed. Algorithm picks roads with greater speed first, preferencing a more pedestrian roads over
// less pedestrian. As result of such heuristic road is not totally the shortest, but it avoids non pedestrian roads, which were
// not marked as "foot=no" in OSM.

double constexpr kSpeedTrunkKMpH = 1.0;
double constexpr kSpeedTrunkLinkKMpH = 1.0;
double constexpr kSpeedPrimaryKMpH = 2.0;
double constexpr kSpeedPrimaryLinkKMpH = 2.0;
double constexpr kSpeedSecondaryKMpH = 3.0;
double constexpr kSpeedSecondaryLinkKMpH = 3.0;
double constexpr kSpeedTertiaryKMpH = 4.0;
double constexpr kSpeedTertiaryLinkKMpH = 4.0;
double constexpr kSpeedServiceKMpH = 5.0;
double constexpr kSpeedUnclassifiedKMpH = 4.5;
double constexpr kSpeedRoadKMpH = 4.0;
double constexpr kSpeedTrackKMpH = 5.0;
double constexpr kSpeedPathKMpH = 5.0;
double constexpr kSpeedBridlewayKMpH = 1.0;
double constexpr kSpeedCyclewayKMpH = 4.0;
double constexpr kSpeedResidentialKMpH = 4.5;
double constexpr kSpeedLivingStreetKMpH = 5.0;
double constexpr kSpeedStepsKMpH = 4.9;
double constexpr kSpeedPedestrianKMpH = 5.0;
double constexpr kSpeedFootwayKMpH = 5.0;
double constexpr kSpeedPlatformKMpH = 5.0;
double constexpr kSpeedPierKMpH = 4.0;
double constexpr kSpeedOffroadKMpH = 3.0;
double constexpr kSpeedFerry = 1.0;

// Default
VehicleModel::LimitsInitList const g_pedestrianLimitsDefault =
{
  // {{roadType, roadType}        {weightSpeedKMpH,         etSpeedKMpH}              passThroughAllowed}
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true},
  {{"highway", "path"},           {kSpeedPathKMpH,          kSpeedPathKMpH},          true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "steps"},          {kSpeedStepsKMpH,         kSpeedStepsKMpH},         true},
  {{"highway", "pedestrian"},     {kSpeedPedestrianKMpH,    kSpeedPedestrianKMpH},    true},
  {{"highway", "footway"},        {kSpeedFootwayKMpH,       kSpeedFootwayKMpH},       true},
  {{"highway", "platform"},       {kSpeedPlatformKMpH,      kSpeedPlatformKMpH},      true}
};

// All options available.
VehicleModel::LimitsInitList const g_pedestrianLimitsAll =
{
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true},
  {{"highway", "path"},           {kSpeedPathKMpH,          kSpeedPathKMpH},          true},
  {{"highway", "bridleway"},      {kSpeedBridlewayKMpH,     kSpeedBridlewayKMpH},     true},
  {{"highway", "cycleway"},       {kSpeedCyclewayKMpH,      kSpeedCyclewayKMpH},      true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "steps"},          {kSpeedStepsKMpH,         kSpeedStepsKMpH},         true},
  {{"highway", "pedestrian"},     {kSpeedPedestrianKMpH,    kSpeedPedestrianKMpH},    true},
  {{"highway", "footway"},        {kSpeedFootwayKMpH,       kSpeedFootwayKMpH},       true},
  {{"highway", "platform"},       {kSpeedPlatformKMpH,      kSpeedPlatformKMpH},      true}
};

// Same as defaults except trunk and trunk link are not allowed.
VehicleModel::LimitsInitList const g_pedestrianLimitsNoTrunk =
{
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true},
  {{"highway", "path"},           {kSpeedPathKMpH,          kSpeedPathKMpH},          true},
  {{"highway", "cycleway"},       {kSpeedCyclewayKMpH,      kSpeedCyclewayKMpH},      true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "steps"},          {kSpeedStepsKMpH,         kSpeedStepsKMpH},         true},
  {{"highway", "pedestrian"},     {kSpeedPedestrianKMpH,    kSpeedPedestrianKMpH},    true},
  {{"highway", "footway"},        {kSpeedFootwayKMpH,       kSpeedFootwayKMpH},       true},
  {{"highway", "platform"},       {kSpeedPlatformKMpH,      kSpeedPlatformKMpH},      true}
};

// Same as defaults except cycleway is allowed.
VehicleModel::LimitsInitList const g_pedestrianLimitsCyclewayAllowed =
{
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true},
  {{"highway", "path"},           {kSpeedPathKMpH,          kSpeedPathKMpH},          true},
  {{"highway", "cycleway"},       {kSpeedCyclewayKMpH,      kSpeedCyclewayKMpH},      true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "steps"},          {kSpeedStepsKMpH,         kSpeedStepsKMpH},         true},
  {{"highway", "pedestrian"},     {kSpeedPedestrianKMpH,    kSpeedPedestrianKMpH},    true},
  {{"highway", "footway"},        {kSpeedFootwayKMpH,       kSpeedFootwayKMpH},       true},
  {{"highway", "platform"},       {kSpeedPlatformKMpH,      kSpeedPlatformKMpH},      true}
};

// Same as defaults except cycleway is allowed and trunk and trunk_link are not allowed.
VehicleModel::LimitsInitList const g_pedestrianLimitsCyclewayAllowedNoTrunk =
{
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,        kSpeedPrimaryKMpH},        true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,    kSpeedPrimaryLinkKMpH},    true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,      kSpeedSecondaryKMpH},      true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH,  kSpeedSecondaryLinkKMpH},  true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,       kSpeedTertiaryKMpH},       true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,   kSpeedTertiaryLinkKMpH},   true},
  {{"highway", "service"},        {kSpeedServiceKMpH,        kSpeedServiceKMpH},        true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,   kSpeedUnclassifiedKMpH},   true},
  {{"highway", "road"},           {kSpeedRoadKMpH,           kSpeedRoadKMpH},           true},
  {{"highway", "track"},          {kSpeedTrackKMpH,          kSpeedTrackKMpH},          true},
  {{"highway", "path"},           {kSpeedPathKMpH,           kSpeedPathKMpH},           true},
  {{"highway", "cycleway"},       {kSpeedCyclewayKMpH,       kSpeedCyclewayKMpH},       true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,    kSpeedResidentialKMpH},    true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,   kSpeedLivingStreetKMpH},   true},
  {{"highway", "steps"},          {kSpeedStepsKMpH,          kSpeedStepsKMpH},          true},
  {{"highway", "pedestrian"},     {kSpeedPedestrianKMpH,     kSpeedPedestrianKMpH},     true},
  {{"highway", "footway"},        {kSpeedFootwayKMpH,        kSpeedFootwayKMpH},        true},
  {{"highway", "platform"},       {kSpeedPlatformKMpH,       kSpeedPlatformKMpH},       true}
};

// Australia
VehicleModel::LimitsInitList const g_pedestrianLimitsAustralia = g_pedestrianLimitsAll;

// Austria
VehicleModel::LimitsInitList const g_pedestrianLimitsAustria = g_pedestrianLimitsNoTrunk;

// Belarus
VehicleModel::LimitsInitList const g_pedestrianLimitsBelarus = g_pedestrianLimitsCyclewayAllowed;

// Belgium
VehicleModel::LimitsInitList const g_pedestrianLimitsBelgium =
{
  // Trunk and trunk_link are not allowed
  // Bridleway and cycleway are allowed
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true},
  {{"highway", "path"},           {kSpeedPathKMpH,          kSpeedPathKMpH},          true},
  {{"highway", "bridleway"},      {kSpeedBridlewayKMpH,     kSpeedBridlewayKMpH},     true},
  {{"highway", "cycleway"},       {kSpeedCyclewayKMpH,      kSpeedCyclewayKMpH},      true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "steps"},          {kSpeedStepsKMpH,         kSpeedStepsKMpH},         true},
  {{"highway", "pedestrian"},     {kSpeedPedestrianKMpH,    kSpeedPedestrianKMpH},    true},
  {{"highway", "footway"},        {kSpeedFootwayKMpH,       kSpeedFootwayKMpH},       true},
  {{"highway", "platform"},       {kSpeedPlatformKMpH,      kSpeedPlatformKMpH},      true}
};

// Brazil
VehicleModel::LimitsInitList const g_pedestrianLimitsBrazil = g_pedestrianLimitsAll;

// Denmark
VehicleModel::LimitsInitList const g_pedestrianLimitsDenmark = g_pedestrianLimitsCyclewayAllowedNoTrunk;

// France
VehicleModel::LimitsInitList const g_pedestrianLimitsFrance = g_pedestrianLimitsNoTrunk;

// Finland
VehicleModel::LimitsInitList const g_pedestrianLimitsFinland = g_pedestrianLimitsCyclewayAllowed;

// Germany
VehicleModel::LimitsInitList const g_pedestrianLimitsGermany = g_pedestrianLimitsDefault;

// Hungary
VehicleModel::LimitsInitList const g_pedestrianLimitsHungary = g_pedestrianLimitsNoTrunk;

// Iceland
VehicleModel::LimitsInitList const g_pedestrianLimitsIceland = g_pedestrianLimitsAll;

// Netherlands
VehicleModel::LimitsInitList const g_pedestrianLimitsNetherlands = g_pedestrianLimitsCyclewayAllowedNoTrunk;

// Norway
VehicleModel::LimitsInitList const g_pedestrianLimitsNorway = g_pedestrianLimitsAll;

// Oman
VehicleModel::LimitsInitList const g_pedestrianLimitsOman = g_pedestrianLimitsAll;

// Poland
VehicleModel::LimitsInitList const g_pedestrianLimitsPoland = g_pedestrianLimitsNoTrunk;

// Romania
VehicleModel::LimitsInitList const g_pedestrianLimitsRomania = g_pedestrianLimitsNoTrunk;

// Russian Federation
VehicleModel::LimitsInitList const g_pedestrianLimitsRussia = g_pedestrianLimitsCyclewayAllowed;

// Slovakia
VehicleModel::LimitsInitList const g_pedestrianLimitsSlovakia = g_pedestrianLimitsNoTrunk;

// Spain
VehicleModel::LimitsInitList const g_pedestrianLimitsSpain = g_pedestrianLimitsNoTrunk;

// Switzerland
VehicleModel::LimitsInitList const g_pedestrianLimitsSwitzerland = g_pedestrianLimitsNoTrunk;

// Turkey
VehicleModel::LimitsInitList const g_pedestrianLimitsTurkey = g_pedestrianLimitsAll;

// Ukraine
VehicleModel::LimitsInitList const g_pedestrianLimitsUkraine = g_pedestrianLimitsNoTrunk;

// United Kingdom
VehicleModel::LimitsInitList const g_pedestrianLimitsUK = g_pedestrianLimitsAll;

// United States of America
VehicleModel::LimitsInitList const g_pedestrianLimitsUS = g_pedestrianLimitsAll;

VehicleModel::SurfaceInitList const g_pedestrianSurface = {
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
  : VehicleModel(classif(), g_pedestrianLimitsDefault, g_pedestrianSurface)
{
  Init();
}

PedestrianModel::PedestrianModel(VehicleModel::LimitsInitList const & speedLimits)
  : VehicleModel(classif(), speedLimits, g_pedestrianSurface)
{
  Init();
}

double PedestrianModel::GetOffroadSpeed() const { return kSpeedOffroadKMpH; }

void PedestrianModel::Init()
{
  initializer_list<char const *> hwtagYesFoot = {"hwtag", "yesfoot"};

  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });
  m_yesFootType = classif().GetTypeByPath(hwtagYesFoot);

  vector<AdditionalRoadTags> const additionalTags = {
    {hwtagYesFoot, m_maxSpeed},
    {{"route", "ferry"}, {kSpeedFerry, kSpeedFerry}},
    {{"man_made", "pier"}, {kSpeedPierKMpH, kSpeedPierKMpH}}
  };

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
  static PedestrianModel const instance(g_pedestrianLimitsAll);
  return instance;
}

PedestrianModelFactory::PedestrianModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<PedestrianModel>(g_pedestrianLimitsDefault);
  m_models["Australia"] = make_shared<PedestrianModel>(g_pedestrianLimitsAustralia);
  m_models["Austria"] = make_shared<PedestrianModel>(g_pedestrianLimitsAustria);
  m_models["Belarus"] = make_shared<PedestrianModel>(g_pedestrianLimitsBelarus);
  m_models["Belgium"] = make_shared<PedestrianModel>(g_pedestrianLimitsBelgium);
  m_models["Brazil"] = make_shared<PedestrianModel>(g_pedestrianLimitsBrazil);
  m_models["Denmark"] = make_shared<PedestrianModel>(g_pedestrianLimitsDenmark);
  m_models["France"] = make_shared<PedestrianModel>(g_pedestrianLimitsFrance);
  m_models["Finland"] = make_shared<PedestrianModel>(g_pedestrianLimitsFinland);
  m_models["Germany"] = make_shared<PedestrianModel>(g_pedestrianLimitsGermany);
  m_models["Hungary"] = make_shared<PedestrianModel>(g_pedestrianLimitsHungary);
  m_models["Iceland"] = make_shared<PedestrianModel>(g_pedestrianLimitsIceland);
  m_models["Netherlands"] = make_shared<PedestrianModel>(g_pedestrianLimitsNetherlands);
  m_models["Norway"] = make_shared<PedestrianModel>(g_pedestrianLimitsNorway);
  m_models["Oman"] = make_shared<PedestrianModel>(g_pedestrianLimitsOman);
  m_models["Poland"] = make_shared<PedestrianModel>(g_pedestrianLimitsPoland);
  m_models["Romania"] = make_shared<PedestrianModel>(g_pedestrianLimitsRomania);
  m_models["Russian Federation"] = make_shared<PedestrianModel>(g_pedestrianLimitsRussia);
  m_models["Slovakia"] = make_shared<PedestrianModel>(g_pedestrianLimitsSlovakia);
  m_models["Spain"] = make_shared<PedestrianModel>(g_pedestrianLimitsSpain);
  m_models["Switzerland"] = make_shared<PedestrianModel>(g_pedestrianLimitsSwitzerland);
  m_models["Turkey"] = make_shared<PedestrianModel>(g_pedestrianLimitsTurkey);
  m_models["Ukraine"] = make_shared<PedestrianModel>(g_pedestrianLimitsUkraine);
  m_models["United Kingdom"] = make_shared<PedestrianModel>(g_pedestrianLimitsUK);
  m_models["United States of America"] = make_shared<PedestrianModel>(g_pedestrianLimitsUS);
}
}  // routing
