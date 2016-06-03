#include "bicycle_model.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/logging.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

namespace
{

// See model specifics in different countries here:
//   http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions
// Document contains proposals for some countries, but we assume that some kinds of roads are ready for bicycle routing,
// but not listed in tables in the document. For example, steps are not listed, paths, roads and services features also
// can be treated as ready for bicycle routing.
// Kinds of roads which we assume are ready for bicycles are marked by // * below.

// See road types here:
//   http://wiki.openstreetmap.org/wiki/Key:highway

// Heuristics:
// For less bicycle roads we add fine by setting smaller value of speed, and for more bicycle roads we
// set greater values of speed. Algorithm picks roads with greater speed first, preferencing a more bicycle roads over
// less bicycle. As result of such heuristic road is not totally the shortest, but it avoids non bicycle roads, which were
// not marked as "hwtag=nobicycle" in OSM.

double constexpr kSpeedTrunkKMpH = 3.0;
double constexpr kSpeedTrunkLinkKMpH = 3.0;
double constexpr kSpeedPrimaryKMpH = 5.0;
double constexpr kSpeedPrimaryLinkKMpH = 5.0;
double constexpr kSpeedSecondaryKMpH = 15.0;
double constexpr kSpeedSecondaryLinkKMpH = 15.0;
double constexpr kSpeedTertiaryKMpH = 15.0;
double constexpr kSpeedTertiaryLinkKMpH = 15.0;
double constexpr kSpeedServiceKMpH = 12.0;
double constexpr kSpeedUnclassifiedKMpH = 12.0;
double constexpr kSpeedRoadKMpH = 10.0;
double constexpr kSpeedTrackKMpH = 8.0;
double constexpr kSpeedPathKMpH = 6.0;
double constexpr kSpeedBridlewayKMpH = 4.0;
double constexpr kSpeedCyclewayKMpH = 15.0;
double constexpr kSpeedResidentialKMpH = 8.0;
double constexpr kSpeedLivingStreetKMpH = 7.0;
double constexpr kSpeedStepsKMpH = 1.0;
double constexpr kSpeedPedestrianKMpH = 5.0;
double constexpr kSpeedFootwayKMpH = 7.0;
double constexpr kSpeedPlatformKMpH = 3.0;

// Default
routing::VehicleModel::InitListT const g_bicycleLimitsDefault =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// All options available.
routing::VehicleModel::InitListT const g_bicycleLimitsAll =
{
  { {"highway", "trunk"},          kSpeedCyclewayKMpH },
  { {"highway", "trunk_link"},     kSpeedCyclewayKMpH },
  { {"highway", "primary"},        kSpeedCyclewayKMpH },
  { {"highway", "primary_link"},   kSpeedCyclewayKMpH },
  { {"highway", "secondary"},      kSpeedCyclewayKMpH },
  { {"highway", "secondary_link"}, kSpeedCyclewayKMpH },
  { {"highway", "tertiary"},       kSpeedCyclewayKMpH },
  { {"highway", "tertiary_link"},  kSpeedCyclewayKMpH },
  { {"highway", "service"},        kSpeedCyclewayKMpH },
  { {"highway", "unclassified"},   kSpeedCyclewayKMpH },
  { {"highway", "road"},           kSpeedCyclewayKMpH },
  { {"highway", "track"},          kSpeedCyclewayKMpH },
  { {"highway", "path"},           kSpeedCyclewayKMpH },
  { {"highway", "bridleway"},      kSpeedCyclewayKMpH },
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedCyclewayKMpH },
  { {"highway", "living_street"},  kSpeedCyclewayKMpH },
  { {"highway", "steps"},          kSpeedCyclewayKMpH },
  { {"highway", "pedestrian"},     kSpeedCyclewayKMpH },
  { {"highway", "footway"},        kSpeedCyclewayKMpH },
  { {"highway", "platform"},       kSpeedCyclewayKMpH },
};

// Australia
routing::VehicleModel::InitListT const g_bicycleLimitsAustralia =
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
routing::VehicleModel::InitListT const g_bicycleLimitsAustria =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Belarus
routing::VehicleModel::InitListT const g_bicycleLimitsBelarus =
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
routing::VehicleModel::InitListT const g_bicycleLimitsBelgium =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH }, // *
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH }, // *
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Brazil
routing::VehicleModel::InitListT const g_bicycleLimitsBrazil =
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
routing::VehicleModel::InitListT const g_bicycleLimitsDenmark =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// France
routing::VehicleModel::InitListT const g_bicycleLimitsFrance =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Finland
routing::VehicleModel::InitListT const g_bicycleLimitsFinland =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Germany
routing::VehicleModel::InitListT const g_bicycleLimitsGermany =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Hungary
routing::VehicleModel::InitListT const g_bicycleLimitsHungary =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Netherlands
routing::VehicleModel::InitListT const g_bicycleLimitsNetherlands =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Norway
routing::VehicleModel::InitListT const g_bicycleLimitsNorway =
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
routing::VehicleModel::InitListT const g_bicycleLimitsPoland =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Romania
routing::VehicleModel::InitListT const g_bicycleLimitsRomania =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Russia
routing::VehicleModel::InitListT const g_bicycleLimitsRussia =
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
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "pedestrian"},     kSpeedPedestrianKMpH },
  { {"highway", "footway"},        kSpeedPedestrianKMpH },
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Slovakia
routing::VehicleModel::InitListT const g_bicycleLimitsSlovakia =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Switzerland
routing::VehicleModel::InitListT const g_bicycleLimitsSwitzerland =
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

// Turkey
routing::VehicleModel::InitListT const g_bicycleLimitsTurkey =
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
  { {"highway", "cycleway"},       kSpeedCyclewayKMpH },
  { {"highway", "residential"},    kSpeedResidentialKMpH },
  { {"highway", "living_street"},  kSpeedLivingStreetKMpH },
  { {"highway", "steps"},          kSpeedStepsKMpH }, // *
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// Ukraine
routing::VehicleModel::InitListT const g_bicycleLimitsUkraine =
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

// United Kingdom
routing::VehicleModel::InitListT const g_bicycleLimitsUK =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

// USA
routing::VehicleModel::InitListT const g_bicycleLimitsUSA =
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
  { {"highway", "platform"},       kSpeedPlatformKMpH }, // *
};

}  // namespace

namespace routing
{

// If one of feature types will be disabled for bicycles, features of this type will be simplyfied
// in generator. Look FeatureBuilder1::IsRoad() for more details.
BicycleModel::BicycleModel()
  : VehicleModel(classif(), g_bicycleLimitsAll)
{
  Init();
}

BicycleModel::BicycleModel(VehicleModel::InitListT const & speedLimits)
  : VehicleModel(classif(), speedLimits)
{
  Init();
}

void BicycleModel::Init()
{
  initializer_list<char const *> hwtagYesbicycle = { "hwtag", "yesbicycle" };

  m_yesBicycleType = classif().GetTypeByPath(hwtagYesbicycle);
  m_noBicycleType = classif().GetTypeByPath({ "hwtag", "nobicycle" });
  m_bicycleBidirType = classif().GetTypeByPath({ "hwtag", "bicycle_bidir" });

  initializer_list<char const *> arr[] =
  {
    hwtagYesbicycle,
    { "route", "ferry" },
    { "man_made", "pier" },
  };

  SetAdditionalRoadTypes(classif(), arr, ARRAY_SIZE(arr));
}

bool BicycleModel::IsNoBicycle(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_noBicycleType) != types.end();
}

bool BicycleModel::IsYesBicycle(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_yesBicycleType) != types.end();
}

bool BicycleModel::IsBicycleBidir(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_bicycleBidirType) != types.end();
}

double BicycleModel::GetSpeed(FeatureType const & f) const
{
  feature::TypesHolder types(f);

  if (IsYesBicycle(types))
    return VehicleModel::GetMaxSpeed();
  if (!IsNoBicycle(types) && HasRoadType(types))
    return VehicleModel::GetMinTypeSpeed(types);

  return 0.0;
}

bool BicycleModel::IsOneWay(FeatureType const & f) const
{
  feature::TypesHolder const types(f);

  if (IsBicycleBidir(types))
    return false;

  return VehicleModel::IsOneWay(f);
}

bool BicycleModel::IsRoad(FeatureType const & f) const
{
  if (f.GetFeatureType() != feature::GEOM_LINE)
    return false;

  feature::TypesHolder types(f);

  if (IsNoBicycle(types))
    return false;
  return VehicleModel::HasRoadType(types);
}

BicycleModelFactory::BicycleModelFactory()
{
  m_models[string()] = make_shared<BicycleModel>(g_bicycleLimitsDefault);
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
  m_models["Netherlands"] = make_shared<BicycleModel>(g_bicycleLimitsNetherlands);
  m_models["Norway"] = make_shared<BicycleModel>(g_bicycleLimitsNorway);
  m_models["Poland"] = make_shared<BicycleModel>(g_bicycleLimitsPoland);
  m_models["Romania"] = make_shared<BicycleModel>(g_bicycleLimitsRomania);
  m_models["Russia"] = make_shared<BicycleModel>(g_bicycleLimitsRussia);
  m_models["Slovakia"] = make_shared<BicycleModel>(g_bicycleLimitsSlovakia);
  m_models["Switzerland"] = make_shared<BicycleModel>(g_bicycleLimitsSwitzerland);
  m_models["Turkey"] = make_shared<BicycleModel>(g_bicycleLimitsTurkey);
  m_models["Ukraine"] = make_shared<BicycleModel>(g_bicycleLimitsUkraine);
  m_models["UK"] = make_shared<BicycleModel>(g_bicycleLimitsUK);
  m_models["USA"] = make_shared<BicycleModel>(g_bicycleLimitsUSA);
}

shared_ptr<IVehicleModel> BicycleModelFactory::GetVehicleModel() const
{
  auto const itr = m_models.find(string());
  ASSERT(itr != m_models.end(), ());
  return itr->second;
}

shared_ptr<IVehicleModel> BicycleModelFactory::GetVehicleModelForCountry(string const & country) const
{
  auto const itr = m_models.find(country);
  if (itr != m_models.end())
  {
    LOG(LDEBUG, ("Bicycle model was found:", country));
    return itr->second;
  }
  LOG(LDEBUG, ("Bicycle model wasn't found, default model is used instead:", country));
  return BicycleModelFactory::GetVehicleModel();
}
}  // routing
