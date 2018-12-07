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
using InOutCitySpeedKMpH = VehicleModel::InOutCitySpeedKMpH;
using SpeedKMpH = VehicleModel::SpeedKMpH;
using MaxspeedFactor = VehicleModel::MaxspeedFactor;

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

// Speed of road features located inside and outside cities and towns polygons in km per hour.
//                                           in city         out city        maxspeed factor is not used
InOutCitySpeedKMpH constexpr kSpeedTrunkKMpH(SpeedKMpH(1.0), SpeedKMpH(1.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTrunkLinkKMpH(SpeedKMpH(1.0), SpeedKMpH(1.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPrimaryKMpH(SpeedKMpH(2.0), SpeedKMpH(2.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPrimaryLinkKMpH(SpeedKMpH(2.0), SpeedKMpH(2.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedSecondaryKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedSecondaryLinkKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTertiaryKMpH(SpeedKMpH(4.0), SpeedKMpH(4.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTertiaryLinkKMpH(SpeedKMpH(4.0), SpeedKMpH(4.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedServiceKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedUnclassifiedKMpH(SpeedKMpH(4.5), SpeedKMpH(4.5), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedRoadKMpH(SpeedKMpH(4.0), SpeedKMpH(4.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTrackKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPathKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedBridlewayKMpH(SpeedKMpH(1.0), SpeedKMpH(1.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedCyclewayKMpH(SpeedKMpH(4.0), SpeedKMpH(4.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedResidentialKMpH(SpeedKMpH(4.5), SpeedKMpH(4.5), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedLivingStreetKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedStepsKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPedestrianKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedFootwayKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPlatformKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPierKMpH(SpeedKMpH(4.0), SpeedKMpH(4.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedFerryKMpH(SpeedKMpH(1.0), SpeedKMpH(1.0), MaxspeedFactor(1.0));

double constexpr kSpeedOffroadKMpH = 3.0;

// Default
VehicleModel::LimitsInitList const g_pedestrianLimitsDefault =
{
  // {{roadType, roadType}        {weightSpeedKMpH, etSpeedKMpH}              passThroughAllowed}
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// All options available.
VehicleModel::LimitsInitList const g_pedestrianLimitsAll =
{
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "bridleway"},      kSpeedBridlewayKMpH,     true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Same as defaults except trunk and trunk link are not allowed.
VehicleModel::LimitsInitList const g_pedestrianLimitsNoTrunk =
{
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Same as defaults except cycleway is allowed.
VehicleModel::LimitsInitList const g_pedestrianLimitsCyclewayAllowed =
{
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Same as defaults except cycleway is allowed and trunk and trunk_link are not allowed.
VehicleModel::LimitsInitList const g_pedestrianLimitsCyclewayAllowedNoTrunk =
{
  {{"highway", "primary"},        kSpeedPrimaryKMpH,        true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,    true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,      true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH,  true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,       true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,   true},
  {{"highway", "service"},        kSpeedServiceKMpH,        true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,   true},
  {{"highway", "road"},           kSpeedRoadKMpH,           true},
  {{"highway", "track"},          kSpeedTrackKMpH,          true},
  {{"highway", "path"},           kSpeedPathKMpH,           true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,       true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,    true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,   true},
  {{"highway", "steps"},          kSpeedStepsKMpH,          true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,     true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,        true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,       true}
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
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "bridleway"},      kSpeedBridlewayKMpH,     true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
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

SpeedKMpH PedestrianModel::GetSpeed(FeatureType & f, SpeedParams const & speedParams) const
{
  return VehicleModel::GetSpeedWihtoutMaxspeed(f, speedParams);
}

double PedestrianModel::GetOffroadSpeed() const { return kSpeedOffroadKMpH; }

void PedestrianModel::Init()
{
  initializer_list<char const *> hwtagYesFoot = {"hwtag", "yesfoot"};

  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });
  m_yesFootType = classif().GetTypeByPath(hwtagYesFoot);

  vector<AdditionalRoadTags> const additionalTags = {
    {hwtagYesFoot, m_modelMaxSpeed},
    {{"route", "ferry"}, kSpeedFerryKMpH},
    {{"man_made", "pier"}, kSpeedPierKMpH}
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
