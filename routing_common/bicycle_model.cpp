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

// Speed of road features located outside cities and towns polygons in km per hour.
double constexpr kOutCitySpeedTrunkKMpH = 3.0;
double constexpr kOutCitySpeedTrunkLinkKMpH = 3.0;
double constexpr kOutCitySpeedPrimaryKMpH = 5.0;
double constexpr kOutCitySpeedPrimaryLinkKMpH = 5.0;
double constexpr kOutCitySpeedSecondaryKMpH = 20.0;
double constexpr kOutCitySpeedSecondaryLinkKMpH = 20.0;
double constexpr kOutCitySpeedTertiaryKMpH = 20.0;
double constexpr kOutCitySpeedTertiaryLinkKMpH = 20.0;
double constexpr kOutCitySpeedServiceKMpH = 12.0;
double constexpr kOutCitySpeedUnclassifiedKMpH = 12.0;
double constexpr kOutCitySpeedRoadKMpH = 10.0;
double constexpr kOutCitySpeedTrackKMpH = 8.0;
double constexpr kOutCitySpeedPathKMpH = 6.0;
double constexpr kOutCitySpeedBridlewayKMpH = 4.0;
double constexpr kOutCitySpeedCyclewayKMpH = 20.0;
double constexpr kOutCitySpeedResidentialKMpH = 8.0;
double constexpr kOutCitySpeedLivingStreetKMpH = 7.0;
double constexpr kOutCitySpeedStepsKMpH = 1.0;
double constexpr kOutCitySpeedPedestrianKMpH = 5.0;
double constexpr kOutCitySpeedFootwayKMpH = 7.0;
double constexpr kOutCitySpeedPlatformKMpH = 3.0;
double constexpr kOutCitySpeedPierKMpH = 7.0;
double constexpr kOutCitySpeedOffroadKMpH = 3.0;
double constexpr kOutCitySpeedFerryKMpH = 3.0;

// Speed of road features located inside cities and towns polygons in km per hour.
double constexpr kInCitySpeedTrunkKMpH = kOutCitySpeedTrunkKMpH;
double constexpr kInCitySpeedTrunkLinkKMpH = kOutCitySpeedTrunkLinkKMpH;
double constexpr kInCitySpeedPrimaryKMpH = 10.0;
double constexpr kInCitySpeedPrimaryLinkKMpH = 10.0;
double constexpr kInCitySpeedSecondaryKMpH = 15.0;
double constexpr kInCitySpeedSecondaryLinkKMpH = 15.0;
double constexpr kInCitySpeedTertiaryKMpH = 15.0;
double constexpr kInCitySpeedTertiaryLinkKMpH = 15.0;
double constexpr kInCitySpeedServiceKMpH = kOutCitySpeedServiceKMpH;
double constexpr kInCitySpeedUnclassifiedKMpH = kOutCitySpeedUnclassifiedKMpH;
double constexpr kInCitySpeedRoadKMpH = kOutCitySpeedRoadKMpH;
double constexpr kInCitySpeedTrackKMpH = kOutCitySpeedTrackKMpH;
double constexpr kInCitySpeedPathKMpH = kOutCitySpeedPathKMpH;
double constexpr kInCitySpeedBridlewayKMpH = kOutCitySpeedBridlewayKMpH;
double constexpr kInCitySpeedCyclewayKMpH = kOutCitySpeedCyclewayKMpH;
double constexpr kInCitySpeedResidentialKMpH = kOutCitySpeedResidentialKMpH;
double constexpr kInCitySpeedLivingStreetKMpH = kOutCitySpeedLivingStreetKMpH;
double constexpr kInCitySpeedStepsKMpH = kOutCitySpeedStepsKMpH;
double constexpr kInCitySpeedPedestrianKMpH = kOutCitySpeedPedestrianKMpH;
double constexpr kInCitySpeedFootwayKMpH = kOutCitySpeedFootwayKMpH;
double constexpr kInCitySpeedPlatformKMpH = kOutCitySpeedPlatformKMpH;
double constexpr kInCitySpeedPierKMpH = kOutCitySpeedPierKMpH;
double constexpr kInCitySpeedOffroadKMpH = kOutCitySpeedOffroadKMpH;
double constexpr kInCitySpeedFerryKMpH = kOutCitySpeedFerryKMpH;

// Default
VehicleModel::LimitsInitList const g_bicycleLimitsDefault =
{
  // {{roadType, roadType}        {weightSpeedKMpH,         etSpeedKMpH}              passThroughAllowed}
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// All options available.
VehicleModel::LimitsInitList const g_bicycleLimitsAll =
{
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "bridleway"},      kOutCitySpeedBridlewayKMpH,     true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kOutCitySpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Same as defaults except trunk and trunk_link are not allowed
VehicleModel::LimitsInitList const g_bicycleLimitsNoTrunk =
{
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Same as defaults except pedestrian is allowed
VehicleModel::LimitsInitList const g_bicycleLimitsPedestrianAllowed =
{
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Same as defaults except bridleway is allowed
VehicleModel::LimitsInitList const g_bicycleLimitsBridlewayAllowed =
{
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "bridleway"},      kOutCitySpeedBridlewayKMpH,     true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Australia
VehicleModel::LimitsInitList const g_bicycleLimitsAustralia = g_bicycleLimitsAll;

// Austria
VehicleModel::LimitsInitList const g_bicycleLimitsAustria =
{
  // No trunk, trunk_link, path
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Belarus
VehicleModel::LimitsInitList const g_bicycleLimitsBelarus =
{
  // Footway and pedestrian are allowed
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kOutCitySpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Belgium
VehicleModel::LimitsInitList const g_bicycleLimitsBelgium =
{
  // No trunk, trunk_link
  // Pedestrian is allowed
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Brazil
VehicleModel::LimitsInitList const g_bicycleLimitsBrazil =
{
  // Bridleway and fotway are allowed
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "bridleway"},      kOutCitySpeedBridlewayKMpH,     true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "footway"},        kOutCitySpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// Denmark
VehicleModel::LimitsInitList const g_bicycleLimitsDenmark = g_bicycleLimitsNoTrunk;

// France
VehicleModel::LimitsInitList const g_bicycleLimitsFrance =
{
  // No trunk, trunk_link
  // Pedestrian is allowed
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
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
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       false},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  false},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
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
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       false},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  false},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "footway"},        kOutCitySpeedFootwayKMpH,       true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
};

// United Kingdom
VehicleModel::LimitsInitList const g_bicycleLimitsUK = g_bicycleLimitsBridlewayAllowed;

// United States of America
VehicleModel::LimitsInitList const g_bicycleLimitsUS =
{
  // Bridleway and pedesprian are allowed
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true},
  {{"highway", "path"},           kOutCitySpeedPathKMpH,          true},
  {{"highway", "bridleway"},      kOutCitySpeedBridlewayKMpH,     true},
  {{"highway", "cycleway"},       kOutCitySpeedCyclewayKMpH,      true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "steps"},          kOutCitySpeedStepsKMpH,         true},
  {{"highway", "pedestrian"},     kOutCitySpeedPedestrianKMpH,    true},
  {{"highway", "platform"},       kOutCitySpeedPlatformKMpH,      true}
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
      {hwtagYesBicycle, m_maxSpeed},
      {{"route", "ferry"}, kOutCitySpeedFerryKMpH},
      {{"man_made", "pier"}, kOutCitySpeedPierKMpH}
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

bool BicycleModel::IsOneWay(FeatureType & f) const
{
  feature::TypesHolder const types(f);

  if (IsBicycleBidir(types))
    return false;

  return VehicleModel::IsOneWay(f);
}

double BicycleModel::GetOffroadSpeed() const { return kOutCitySpeedOffroadKMpH; }

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
