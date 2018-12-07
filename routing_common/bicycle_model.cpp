#include "routing_common/bicycle_model.hpp"

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
// Document contains proposals for some countries, but we assume that some kinds of roads are ready for bicycle routing,
// but not listed in tables in the document. For example, steps are not listed, paths, roads and services features also
// can be treated as ready for bicycle routing. These road types were added to lists below.

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

// Heuristics:
// For less bicycle roads we add fine by setting smaller value of speed, and for more bicycle roads we
// set greater values of speed. Algorithm picks roads with greater speed first, preferencing a more bicycle roads over
// less bicycle. As result of such heuristic road is not totally the shortest, but it avoids non bicycle roads, which were
// not marked as "hwtag=nobicycle" in OSM.

// Speed of road features located inside and outside cities and towns polygons in km per hour.
//                                           in city         out city        maxspeed factor is not used
InOutCitySpeedKMpH constexpr kSpeedTrunkKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTrunkLinkKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPrimaryKMpH(SpeedKMpH(10.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPrimaryLinkKMpH(SpeedKMpH(10.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedSecondaryKMpH(SpeedKMpH(15.0), SpeedKMpH(20.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedSecondaryLinkKMpH(SpeedKMpH(15.0), SpeedKMpH(20.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTertiaryKMpH(SpeedKMpH(15.0), SpeedKMpH(20.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTertiaryLinkKMpH(SpeedKMpH(15.0), SpeedKMpH(20.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedServiceKMpH(SpeedKMpH(12.0), SpeedKMpH(12.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedUnclassifiedKMpH(SpeedKMpH(12.0), SpeedKMpH(12.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedRoadKMpH(SpeedKMpH(10.0), SpeedKMpH(10.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedTrackKMpH(SpeedKMpH(8.0), SpeedKMpH(8.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPathKMpH(SpeedKMpH(6.0), SpeedKMpH(6.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedBridlewayKMpH(SpeedKMpH(4.0), SpeedKMpH(4.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedCyclewayKMpH(SpeedKMpH(20.0), SpeedKMpH(20.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedResidentialKMpH(SpeedKMpH(8.0), SpeedKMpH(8.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedLivingStreetKMpH(SpeedKMpH(7.0), SpeedKMpH(7.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedStepsKMpH(SpeedKMpH(1.0), SpeedKMpH(1.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPedestrianKMpH(SpeedKMpH(5.0), SpeedKMpH(5.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedFootwayKMpH(SpeedKMpH(7.0), SpeedKMpH(7.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPlatformKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedPierKMpH(SpeedKMpH(7.0), SpeedKMpH(7.0), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr kSpeedFerryKMpH(SpeedKMpH(3.0), SpeedKMpH(3.0), MaxspeedFactor(1.0));

double constexpr kSpeedOffroadKMpH = 3.0;

// Default
VehicleModel::LimitsInitList const g_bicycleLimitsDefault =
{
// {{roadType, roadType}        Speed km per hour    passThroughAllowed}
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// All options available.
VehicleModel::LimitsInitList const g_bicycleLimitsAll =
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

// Same as defaults except trunk and trunk_link are not allowed
VehicleModel::LimitsInitList const g_bicycleLimitsNoTrunk =
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Same as defaults except pedestrian is allowed
VehicleModel::LimitsInitList const g_bicycleLimitsPedestrianAllowed =
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Same as defaults except bridleway is allowed
VehicleModel::LimitsInitList const g_bicycleLimitsBridlewayAllowed =
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Australia
VehicleModel::LimitsInitList const g_bicycleLimitsAustralia = g_bicycleLimitsAll;

// Austria
VehicleModel::LimitsInitList const g_bicycleLimitsAustria =
{
  // No trunk, trunk_link, path
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
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Belarus
VehicleModel::LimitsInitList const g_bicycleLimitsBelarus =
{
  // Footway and pedestrian are allowed
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

// Belgium
VehicleModel::LimitsInitList const g_bicycleLimitsBelgium =
{
  // No trunk, trunk_link
  // Pedestrian is allowed
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Brazil
VehicleModel::LimitsInitList const g_bicycleLimitsBrazil =
{
  // Bridleway and fotway are allowed
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
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Denmark
VehicleModel::LimitsInitList const g_bicycleLimitsDenmark = g_bicycleLimitsNoTrunk;

// France
VehicleModel::LimitsInitList const g_bicycleLimitsFrance =
{
  // No trunk, trunk_link
  // Pedestrian is allowed
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Finland
VehicleModel::LimitsInitList const g_bicycleLimitsFinland = g_bicycleLimitsPedestrianAllowed;

// Germany
VehicleModel::LimitsInitList const g_bicycleLimitsGermany = g_bicycleLimitsDefault;

// Hungary
VehicleModel::LimitsInitList const g_bicycleLimitsHungary = g_bicycleLimitsNoTrunk;

// Iceland
VehicleModel::LimitsInitList const g_bicycleLimitsIceland = g_bicycleLimitsAll;

// Netherlands
VehicleModel::LimitsInitList const g_bicycleLimitsNetherlands = g_bicycleLimitsNoTrunk;

// Norway
VehicleModel::LimitsInitList const g_bicycleLimitsNorway = g_bicycleLimitsAll;

// Oman
VehicleModel::LimitsInitList const g_bicycleLimitsOman = g_bicycleLimitsBridlewayAllowed;

// Poland
VehicleModel::LimitsInitList const g_bicycleLimitsPoland = g_bicycleLimitsNoTrunk;

// Romania
VehicleModel::LimitsInitList const g_bicycleLimitsRomania = g_bicycleLimitsNoTrunk;

// Russian Federation
VehicleModel::LimitsInitList const g_bicycleLimitsRussia =
{
  // Footway and pedestrian are allowed
  // No pass through service and living_street
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       false},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  false},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedPedestrianKMpH,    true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// Slovakia
VehicleModel::LimitsInitList const g_bicycleLimitsSlovakia = g_bicycleLimitsNoTrunk;

// Spain
VehicleModel::LimitsInitList const g_bicycleLimitsSpain = g_bicycleLimitsPedestrianAllowed;

// Switzerland
VehicleModel::LimitsInitList const g_bicycleLimitsSwitzerland = g_bicycleLimitsNoTrunk;

// Turkey
VehicleModel::LimitsInitList const g_bicycleLimitsTurkey = g_bicycleLimitsDefault;

// Ukraine
VehicleModel::LimitsInitList const g_bicycleLimitsUkraine =
{
  // No trunk
  // Footway and perestrian are allowed
  // No pass through living_street and service
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       false},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true},
  {{"highway", "path"},           kSpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kSpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  false},
  {{"highway", "steps"},          kSpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kSpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

// United Kingdom
VehicleModel::LimitsInitList const g_bicycleLimitsUK = g_bicycleLimitsBridlewayAllowed;

// United States of America
VehicleModel::LimitsInitList const g_bicycleLimitsUS =
{
  // Bridleway and pedesprian are allowed
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
  {{"highway", "platform"},       kSpeedPlatformKMpH,      true}
};

VehicleModel::SurfaceInitList const g_bicycleSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {0.8, 0.8}},
  {{"psurface", "unpaved_good"}, {1.0, 1.0}},
  {{"psurface", "unpaved_bad"}, {0.3, 0.3}}
};
}  // namespace

namespace routing
{
BicycleModel::BicycleModel() : VehicleModel(classif(), g_bicycleLimitsDefault, g_bicycleSurface)
{
  Init();
}

BicycleModel::BicycleModel(VehicleModel::LimitsInitList const & speedLimits)
  : VehicleModel(classif(), speedLimits, g_bicycleSurface)
{
  Init();
}

void BicycleModel::Init()
{
  initializer_list<char const *> hwtagYesBicycle = {"hwtag", "yesbicycle"};

  m_yesBicycleType = classif().GetTypeByPath(hwtagYesBicycle);
  m_noBicycleType = classif().GetTypeByPath({"hwtag", "nobicycle"});
  m_bidirBicycleType = classif().GetTypeByPath({"hwtag", "bidir_bicycle"});

  vector<AdditionalRoadTags> const additionalTags = {
      {hwtagYesBicycle, m_modelMaxSpeed},
      {{"route", "ferry"}, kSpeedFerryKMpH},
      {{"man_made", "pier"}, kSpeedPierKMpH}
  };

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

SpeedKMpH BicycleModel::GetSpeed(FeatureType & f, SpeedParams const & speedParams) const
{
  return VehicleModel::GetSpeedWihtoutMaxspeed(f, speedParams);
}

bool BicycleModel::IsOneWay(FeatureType & f) const
{
  feature::TypesHolder const types(f);

  if (IsBicycleBidir(types))
    return false;

  return VehicleModel::IsOneWay(f);
}

double BicycleModel::GetOffroadSpeed() const { return kSpeedOffroadKMpH; }

// If one of feature types will be disabled for bicycles, features of this type will be simplyfied
// in generator. Look FeatureBuilder1::IsRoad() for more details.
// static
BicycleModel const & BicycleModel::AllLimitsInstance()
{
  static BicycleModel const instance(g_bicycleLimitsAll);
  return instance;
}

BicycleModelFactory::BicycleModelFactory(
    CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<BicycleModel>(g_bicycleLimitsDefault);
  m_models["Australia"] = make_shared<BicycleModel>(g_bicycleLimitsAustralia);
  m_models["Austria"] = make_shared<BicycleModel>(g_bicycleLimitsAustria);
  m_models["Belarus"] = make_shared<BicycleModel>(g_bicycleLimitsBelarus);
  m_models["Belgium"] = make_shared<BicycleModel>(g_bicycleLimitsBelgium);
  m_models["Brazil"] = make_shared<BicycleModel>(g_bicycleLimitsBrazil);
  m_models["Denmark"] = make_shared<BicycleModel>(g_bicycleLimitsDenmark);
  m_models["France"] = make_shared<BicycleModel>(g_bicycleLimitsFrance);
  m_models["Finland"] = make_shared<BicycleModel>(g_bicycleLimitsFinland);
  m_models["Germany"] = make_shared<BicycleModel>(g_bicycleLimitsGermany);
  m_models["Hungary"] = make_shared<BicycleModel>(g_bicycleLimitsHungary);
  m_models["Iceland"] = make_shared<BicycleModel>(g_bicycleLimitsIceland);
  m_models["Netherlands"] = make_shared<BicycleModel>(g_bicycleLimitsNetherlands);
  m_models["Norway"] = make_shared<BicycleModel>(g_bicycleLimitsNorway);
  m_models["Oman"] = make_shared<BicycleModel>(g_bicycleLimitsOman);
  m_models["Poland"] = make_shared<BicycleModel>(g_bicycleLimitsPoland);
  m_models["Romania"] = make_shared<BicycleModel>(g_bicycleLimitsRomania);
  m_models["Russian Federation"] = make_shared<BicycleModel>(g_bicycleLimitsRussia);
  m_models["Slovakia"] = make_shared<BicycleModel>(g_bicycleLimitsSlovakia);
  m_models["Spain"] = make_shared<BicycleModel>(g_bicycleLimitsSpain);
  m_models["Switzerland"] = make_shared<BicycleModel>(g_bicycleLimitsSwitzerland);
  m_models["Turkey"] = make_shared<BicycleModel>(g_bicycleLimitsTurkey);
  m_models["Ukraine"] = make_shared<BicycleModel>(g_bicycleLimitsUkraine);
  m_models["United Kingdom"] = make_shared<BicycleModel>(g_bicycleLimitsUK);
  m_models["United States of America"] = make_shared<BicycleModel>(g_bicycleLimitsUS);
}
}  // routing
