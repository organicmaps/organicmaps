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
// Despite we assume trunks, primary and secondary roads footable with some fine,
// we assume links trunk_link, primary_link and secondary_link are prohibited.

double constexpr kSpeedTrunkKMpH = 1.0;
double constexpr kSpeedTrunkLinkKMpH = 0.0;
double constexpr kSpeedPrimaryKMpH = 1.0;
double constexpr kSpeedPrimaryLinkKMpH = 0.0;
double constexpr kSpeedSecondaryKMpH = 2.0;
double constexpr kSpeedSecondaryLinkKMpH = 0.0;
double constexpr kSpeedTertiaryKMpH = 2.25;
double constexpr kSpeedTertiaryLinkKMpH = 1.0;
double constexpr kSpeedServiceKMpH = 3.0;
double constexpr kSpeedUnclassifiedKMpH = 3.5;
double constexpr kSpeedRoadKMpH = 4.0;
double constexpr kSpeedTrackKMpH = 4.0;
double constexpr kSpeedPathKMpH = 4.25;
double constexpr kSpeedBridlewayKMpH = 4.0;
double constexpr kSpeedCyclewayKMpH = 4.25;
double constexpr kSpeedResidentialKMpH = 5.0;
double constexpr kSpeedLivingStreetKMpH = 5.0;
double constexpr kSpeedStepsKMpH = 5.0;
double constexpr kSpeedPedestrianKMpH = 5.0;
double constexpr kSpeedFootwayKMpH = 5.0;

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
};

}  // namespace

namespace routing
{

PedestrianModel::PedestrianModel()
  : VehicleModel(classif(), s_pedestrianLimits_Default)
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

  initializer_list<char const *> arr[] =
  {
    { "route", "ferry" },
    { "man_made", "pier" },
  };

  SetAdditionalRoadTypes(classif(), arr, ARRAY_SIZE(arr));
}

bool PedestrianModel::IsFoot(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_noFootType) == types.end();
}

double PedestrianModel::GetSpeed(FeatureType const & f) const
{
  feature::TypesHolder types(f);

  if (IsFoot(types) && IsRoad(types))
    return VehicleModel::GetSpeed(types);

  return 0.0;
}


PedestrianModelFactory::PedestrianModelFactory()
{
  m_models[string()] = make_shared<PedestrianModel>();
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
