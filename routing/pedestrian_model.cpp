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
routing::VehicleModel::InitListT const s_pedestrianLimits_Default =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_All =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Australia =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Austria =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Belarus =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Belgium =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Brazil =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Denmark =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_France =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Finland =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Germany =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Hungary =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Netherlands =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Norway =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Poland =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Romania =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Russia =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Slovakia =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Switzerland =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Turkey =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_Ukraine =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_UK =
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
routing::VehicleModel::InitListT const s_pedestrianLimits_USA =
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
PedestrianModel::PedestrianModel() : VehicleModel(classif(), s_pedestrianLimits_All)
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
  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });
  m_yesFootType = classif().GetTypeByPath({ "hwtag", "yesfoot" });

  initializer_list<char const *> arr[] =
  {
    { "route", "ferry" },
    { "man_made", "pier" },
  };

  SetAdditionalRoadTypes(classif(), arr, ARRAY_SIZE(arr));
}

bool PedestrianModel::IsNoFoot(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_noFootType) != types.end();
}

bool PedestrianModel::IsYesFoot(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_yesFootType) != types.end();
}

double PedestrianModel::GetSpeed(FeatureType const & f) const
{
  feature::TypesHolder types(f);

  if (IsYesFoot(types))
    return VehicleModel::GetMaxSpeed();
  if (!IsNoFoot(types) && IsRoad(types))
    return VehicleModel::GetSpeed(types);

  return 0.0;
}


PedestrianModelFactory::PedestrianModelFactory()
{
  m_models[string()] = make_shared<PedestrianModel>(s_pedestrianLimits_Default);
  m_models["Australia"] = make_shared<PedestrianModel>(s_pedestrianLimits_Australia);
  m_models["Austria"] = make_shared<PedestrianModel>(s_pedestrianLimits_Austria);
  m_models["Belarus"] = make_shared<PedestrianModel>(s_pedestrianLimits_Belarus);
  m_models["Belgium"] = make_shared<PedestrianModel>(s_pedestrianLimits_Belgium);
  m_models["Brazil"] = make_shared<PedestrianModel>(s_pedestrianLimits_Brazil);
  m_models["Denmark"] = make_shared<PedestrianModel>(s_pedestrianLimits_Denmark);
  m_models["France"] = make_shared<PedestrianModel>(s_pedestrianLimits_France);
  m_models["Finland"] = make_shared<PedestrianModel>(s_pedestrianLimits_Finland);
  m_models["Germany"] = make_shared<PedestrianModel>(s_pedestrianLimits_Germany);
  m_models["Hungary"] = make_shared<PedestrianModel>(s_pedestrianLimits_Hungary);
  m_models["Netherlands"] = make_shared<PedestrianModel>(s_pedestrianLimits_Netherlands);
  m_models["Norway"] = make_shared<PedestrianModel>(s_pedestrianLimits_Norway);
  m_models["Poland"] = make_shared<PedestrianModel>(s_pedestrianLimits_Poland);
  m_models["Romania"] = make_shared<PedestrianModel>(s_pedestrianLimits_Romania);
  m_models["Russia"] = make_shared<PedestrianModel>(s_pedestrianLimits_Russia);
  m_models["Slovakia"] = make_shared<PedestrianModel>(s_pedestrianLimits_Slovakia);
  m_models["Switzerland"] = make_shared<PedestrianModel>(s_pedestrianLimits_Switzerland);
  m_models["Turkey"] = make_shared<PedestrianModel>(s_pedestrianLimits_Turkey);
  m_models["Ukraine"] = make_shared<PedestrianModel>(s_pedestrianLimits_Ukraine);
  m_models["UK"] = make_shared<PedestrianModel>(s_pedestrianLimits_UK);
  m_models["USA"] = make_shared<PedestrianModel>(s_pedestrianLimits_USA);
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
