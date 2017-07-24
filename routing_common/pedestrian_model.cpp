#include "routing_common/pedestrian_model.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/logging.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

namespace
{

// See model specifics in different countries here:
//   http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions
// Document contains proposals for some countries, but we assume that some kinds of roads are ready for pedestrian routing,
// but not listed in tables in the document. For example, steps are not listed, paths, roads and services features also
// can be treated as ready for pedestrian routing.
// Kinds of roads which we assume footable are marked by // * below.

// See road types here:
//   http://wiki.openstreetmap.org/wiki/Key:highway

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

// Default
routing::VehicleModel::InitListT const g_pedestrianLimitsDefault =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// All options available.
routing::VehicleModel::InitListT const g_pedestrianLimitsAll =
{
  { {"highway", "trunk"},          kSpeedPedestrianKMpH,    true },
  { {"highway", "trunk_link"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "primary"},        kSpeedPedestrianKMpH,    true },
  { {"highway", "primary_link"},   kSpeedPedestrianKMpH,    true },
  { {"highway", "secondary"},      kSpeedPedestrianKMpH,    true },
  { {"highway", "secondary_link"}, kSpeedPedestrianKMpH,    true },
  { {"highway", "tertiary"},       kSpeedPedestrianKMpH,    true },
  { {"highway", "tertiary_link"},  kSpeedPedestrianKMpH,    true },
  { {"highway", "service"},        kSpeedPedestrianKMpH,    true },
  { {"highway", "unclassified"},   kSpeedPedestrianKMpH,    true },
  { {"highway", "road"},           kSpeedPedestrianKMpH,    true },
  { {"highway", "track"},          kSpeedPedestrianKMpH,    true },
  { {"highway", "path"},           kSpeedPedestrianKMpH,    true },
  { {"highway", "bridleway"},      kSpeedPedestrianKMpH,    true },
  { {"highway", "cycleway"},       kSpeedPedestrianKMpH,    true },
  { {"highway", "residential"},    kSpeedPedestrianKMpH,    true },
  { {"highway", "living_street"},  kSpeedPedestrianKMpH,    true },
  { {"highway", "steps"},          kSpeedPedestrianKMpH,    true },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedPedestrianKMpH,    true },
  { {"highway", "platform"},       kSpeedPedestrianKMpH,    true },
};

// Australia
routing::VehicleModel::InitListT const g_pedestrianLimitsAustralia =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true }, // *
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Austria
routing::VehicleModel::InitListT const g_pedestrianLimitsAustria =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Belarus
routing::VehicleModel::InitListT const g_pedestrianLimitsBelarus =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true }, // *
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true }, // *
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Belgium
routing::VehicleModel::InitListT const g_pedestrianLimitsBelgium =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true }, // *
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true }, // *
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true }, // *
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Brazil
routing::VehicleModel::InitListT const g_pedestrianLimitsBrazil =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Denmark
routing::VehicleModel::InitListT const g_pedestrianLimitsDenmark =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true }, // *
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// France
routing::VehicleModel::InitListT const g_pedestrianLimitsFrance =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true }, // *
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Finland
routing::VehicleModel::InitListT const g_pedestrianLimitsFinland =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true }, // *
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Germany
routing::VehicleModel::InitListT const g_pedestrianLimitsGermany =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Hungary
routing::VehicleModel::InitListT const g_pedestrianLimitsHungary =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true }, // *
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Netherlands
routing::VehicleModel::InitListT const g_pedestrianLimitsNetherlands =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Norway
routing::VehicleModel::InitListT const g_pedestrianLimitsNorway =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Poland
routing::VehicleModel::InitListT const g_pedestrianLimitsPoland =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Romania
routing::VehicleModel::InitListT const g_pedestrianLimitsRomania =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Russia
routing::VehicleModel::InitListT const g_pedestrianLimitsRussia =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Slovakia
routing::VehicleModel::InitListT const g_pedestrianLimitsSlovakia =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Switzerland
routing::VehicleModel::InitListT const g_pedestrianLimitsSwitzerland =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Turkey
routing::VehicleModel::InitListT const g_pedestrianLimitsTurkey =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// Ukraine
routing::VehicleModel::InitListT const g_pedestrianLimitsUkraine =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true },
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// United Kingdom
routing::VehicleModel::InitListT const g_pedestrianLimitsUK =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true }, // *
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

// USA
routing::VehicleModel::InitListT const g_pedestrianLimitsUS =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH,         true },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true },
  { {"highway", "primary"},        kSpeedPrimaryKMpH,       true },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH,     true },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH,      true },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true },
  { {"highway", "service"},        kSpeedServiceKMpH,       true }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true },
  { {"highway", "road"},           kSpeedRoadKMpH,          true },
  { {"highway", "track"},          kSpeedTrackKMpH,         true }, // *
  { {"highway", "path"},           kSpeedPathKMpH,          true },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH,     true },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH,      true },
  { {"highway", "residential"},    kSpeedResidentialKMpH,   true },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH,  true },
  { {"highway", "steps"},          kSpeedStepsKMpH,         true }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH,    true },
  { {"highway", "footway"},        kSpeedFootwayKMpH,       true },
  { {"highway", "platform"},       kSpeedPlatformKMpH,      true }, // *
};

}  // namespace

namespace routing
{

// If one of feature types will be disabled for pedestrian, features of this type will be simplyfied
// in generator. Look FeatureBuilder1::IsRoad() for more details.
PedestrianModel::PedestrianModel()
  : VehicleModel(classif(), g_pedestrianLimitsAll)
{
  Init();
}

PedestrianModel::PedestrianModel(VehicleModel::InitListT const & speedLimits)
  : VehicleModel(classif(), speedLimits)
{
  Init();
}

void PedestrianModel::Init()
{
  initializer_list<char const *> hwtagYesFoot = {"hwtag", "yesfoot"};

  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });
  m_yesFootType = classif().GetTypeByPath(hwtagYesFoot);

  vector<AdditionalRoadTags> const additionalTags = {
      {hwtagYesFoot, m_maxSpeedKMpH},
      {{"route", "ferry"}, m_maxSpeedKMpH},
      {{"man_made", "pier"}, kSpeedPierKMpH},
  };

  SetAdditionalRoadTypes(classif(), additionalTags);
}

IVehicleModel::RoadAvailability PedestrianModel::GetRoadAvailability(feature::TypesHolder const & types) const
{
  if (types.Has(m_yesFootType))
    return RoadAvailability::Available;
  if (types.Has(m_noFootType))
    return RoadAvailability::NotAvailable;
  return RoadAvailability::Unknown;
}

// static
PedestrianModel const & PedestrianModel::AllLimitsInstance()
{
  static PedestrianModel const instance;
  return instance;
}

PedestrianModelFactory::PedestrianModelFactory()
{
  m_models[string()] = make_shared<PedestrianModel>(g_pedestrianLimitsDefault);
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
  m_models["Netherlands"] = make_shared<PedestrianModel>(g_pedestrianLimitsNetherlands);
  m_models["Norway"] = make_shared<PedestrianModel>(g_pedestrianLimitsNorway);
  m_models["Poland"] = make_shared<PedestrianModel>(g_pedestrianLimitsPoland);
  m_models["Romania"] = make_shared<PedestrianModel>(g_pedestrianLimitsRomania);
  m_models["Russia"] = make_shared<PedestrianModel>(g_pedestrianLimitsRussia);
  m_models["Slovakia"] = make_shared<PedestrianModel>(g_pedestrianLimitsSlovakia);
  m_models["Switzerland"] = make_shared<PedestrianModel>(g_pedestrianLimitsSwitzerland);
  m_models["Turkey"] = make_shared<PedestrianModel>(g_pedestrianLimitsTurkey);
  m_models["Ukraine"] = make_shared<PedestrianModel>(g_pedestrianLimitsUkraine);
  m_models["UK"] = make_shared<PedestrianModel>(g_pedestrianLimitsUK);
  m_models["US"] = make_shared<PedestrianModel>(g_pedestrianLimitsUS);
}

shared_ptr<IVehicleModel> PedestrianModelFactory::GetVehicleModel() const
{
  auto const itr = m_models.find(string());
  ASSERT(itr != m_models.end(), ());
  return itr->second;
}

shared_ptr<IVehicleModel> PedestrianModelFactory::GetVehicleModelForCountry(string const & country) const
{
  auto const itr = m_models.find(country);
  if (itr != m_models.end())
  {
    LOG(LDEBUG, ("Pedestrian model was found:", country));
    return itr->second;
  }
  LOG(LDEBUG, ("Pedestrian model wasn't found, default model is used instead:", country));
  return PedestrianModelFactory::GetVehicleModel();
}
}  // routing
