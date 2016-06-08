#include "pedestrian_model.hpp"

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

// Default
routing::VehicleModel::InitListT const g_pedestrianLimitsDefault =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// All options available.
routing::VehicleModel::InitListT const g_pedestrianLimitsAll =
{
  { {"highway", "trunk"},          kSpeedPedestrianKMpH },
  { {"highway", "trunk_link"},     kSpeedPedestrianKMpH },
  { {"highway", "primary"},        kSpeedPedestrianKMpH },
  { {"highway", "primary_link"},   kSpeedPedestrianKMpH },
  { {"highway", "secondary"},      kSpeedPedestrianKMpH },
  { {"highway", "secondary_link"}, kSpeedPedestrianKMpH },
  { {"highway", "tertiary"},       kSpeedPedestrianKMpH },
  { {"highway", "tertiary_link"},  kSpeedPedestrianKMpH },
  { {"highway", "service"},        kSpeedPedestrianKMpH },
  { {"highway", "unclassified"},   kSpeedPedestrianKMpH },
  { {"highway", "road"},           kSpeedPedestrianKMpH },
  { {"highway", "track"},          kSpeedPedestrianKMpH },
  { {"highway", "path"},           kSpeedPedestrianKMpH },
  { {"highway", "bridleway"},      kSpeedPedestrianKMpH },
  { {"highway", "cycleway"},       kSpeedPedestrianKMpH },
  { {"highway", "residential"},    kSpeedPedestrianKMpH },
  { {"highway", "living_street"},  kSpeedPedestrianKMpH },
  { {"highway", "steps"},          kSpeedPedestrianKMpH },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedPedestrianKMpH },
  { {"highway", "platform"},       kSpeedPedestrianKMpH },
};

// Australia
routing::VehicleModel::InitListT const g_pedestrianLimitsAustralia =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH }, // *
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Austria
routing::VehicleModel::InitListT const g_pedestrianLimitsAustria =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Belarus
routing::VehicleModel::InitListT const g_pedestrianLimitsBelarus =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH }, // *
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH }, // *
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Belgium
routing::VehicleModel::InitListT const g_pedestrianLimitsBelgium =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH }, // *
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH }, // *
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH }, // *
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Brazil
routing::VehicleModel::InitListT const g_pedestrianLimitsBrazil =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Denmark
routing::VehicleModel::InitListT const g_pedestrianLimitsDenmark =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH }, // *
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// France
routing::VehicleModel::InitListT const g_pedestrianLimitsFrance =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH }, // *
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Finland
routing::VehicleModel::InitListT const g_pedestrianLimitsFinland =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH }, // *
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Germany
routing::VehicleModel::InitListT const g_pedestrianLimitsGermany =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Hungary
routing::VehicleModel::InitListT const g_pedestrianLimitsHungary =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH }, // *
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Netherlands
routing::VehicleModel::InitListT const g_pedestrianLimitsNetherlands =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Norway
routing::VehicleModel::InitListT const g_pedestrianLimitsNorway =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Poland
routing::VehicleModel::InitListT const g_pedestrianLimitsPoland =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Romania
routing::VehicleModel::InitListT const g_pedestrianLimitsRomania =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Russia
routing::VehicleModel::InitListT const g_pedestrianLimitsRussia =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Slovakia
routing::VehicleModel::InitListT const g_pedestrianLimitsSlovakia =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Switzerland
routing::VehicleModel::InitListT const g_pedestrianLimitsSwitzerland =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Turkey
routing::VehicleModel::InitListT const g_pedestrianLimitsTurkey =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Ukraine
routing::VehicleModel::InitListT const g_pedestrianLimitsUkraine =
{
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH },
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH },
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH },
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// United Kingdom
routing::VehicleModel::InitListT const g_pedestrianLimitsUK =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH }, // *
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// USA
routing::VehicleModel::InitListT const g_pedestrianLimitsUS =
{
  { {"highway", "trunk"},          kSpeedTrunkKMpH },
  { {"highway", "trunk_link"},     kSpeedTrunkLinkKMpH },
  { {"highway", "primary"},        kSpeedPrimaryKMpH },
  { {"highway", "primary_link"},   kSpeedPrimaryLinkKMpH },
  { {"highway", "secondary"},      kSpeedSecondaryKMpH },
  { {"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH },
  { {"highway", "tertiary"},       kSpeedTertiaryKMpH },
  { {"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH },
  { {"highway", "service"},        kSpeedServiceKMpH }, // *
  { {"highway", "unclassified"},   kSpeedUnclassifiedKMpH },
  { {"highway", "road"},           kSpeedRoadKMpH },
  { {"highway", "track"},          kSpeedTrackKMpH }, // *
  { {"highway", "path"},           kSpeedPathKMpH },
  { {"highway", "bridleway"},      kSpeedBridlewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedFootwayKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
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
  initializer_list<char const *> hwtagYesfoot = {"hwtag", "yesfoot"};

  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });
  m_yesFootType = classif().GetTypeByPath(hwtagYesfoot);

  initializer_list<char const *> arr[] = {
      hwtagYesfoot, {"route", "ferry"}, {"man_made", "pier"},
  };

  SetAdditionalRoadTypes(classif(), arr, ARRAY_SIZE(arr));
}

VehicleModel::Restriction PedestrianModel::IsPedestrianAllowed(feature::TypesHolder const & types) const
{
  if (types.Has(m_yesFootType))
    return Restriction::Yes;
  if (types.Has(m_noFootType))
    return Restriction::No;
  return Restriction::Unknown;
}

double PedestrianModel::GetSpeed(FeatureType const & f) const
{
  feature::TypesHolder const types(f);

  Restriction const restriction = IsPedestrianAllowed(types);
  if (restriction == Restriction::Yes)
    return VehicleModel::GetMaxSpeed();
  if (restriction != Restriction::No && HasRoadType(types))
    return VehicleModel::GetMinTypeSpeed(types);

  return 0.0;
}

bool PedestrianModel::IsRoad(FeatureType const & f) const
{
  if (f.GetFeatureType() != feature::GEOM_LINE)
    return false;

  feature::TypesHolder const types(f);

  if (IsPedestrianAllowed(types) == Restriction::No)
    return false;

  return VehicleModel::HasRoadType(types);
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
