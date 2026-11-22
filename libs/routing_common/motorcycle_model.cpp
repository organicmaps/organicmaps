#include "routing_common/motorcycle_model.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"

namespace motorcycle_model
{
using namespace routing;

// Same offroad fake-edge semantics as CarModel: heavy weight to snap to the nearest road,
// 0 seconds added to ETA.
SpeedKMpH constexpr kSpeedOffroadKMpH = {0.01 /* weight */, kNotUsed /* eta */};

// Matches CarModel road types because motorcycle routing uses the VehicleType::Car MWM routing graph.
// HighwayPath is intentionally excluded (not indexed in kCarMask unless tagged hwtag=yescar),
// otherwise start/finish snapping can pick unindexed paths and fail with RouteNotFound.
VehicleModel::LimitsInitList const kDefaultOptions = {
    HighwayType::HighwayMotorway,     HighwayType::HighwayMotorwayLink,  HighwayType::HighwayTrunk,
    HighwayType::HighwayTrunkLink,    HighwayType::HighwayPrimary,       HighwayType::HighwayPrimaryLink,
    HighwayType::HighwaySecondary,    HighwayType::HighwaySecondaryLink, HighwayType::HighwayTertiary,
    HighwayType::HighwayTertiaryLink, HighwayType::HighwayResidential,   HighwayType::HighwayUnclassified,
    HighwayType::HighwayService,      HighwayType::HighwayLivingStreet,  HighwayType::HighwayRoad,
    HighwayType::HighwayTrack,        HighwayType::RouteShuttleTrain,    HighwayType::RouteFerry,
    HighwayType::ManMadePier};

// Realistic base speeds (in city, out of city) used for both weight and ETA before profile factors.
HighwayBasedSpeeds const kHighwayBasedSpeeds = {
    // Tier 1:
    {HighwayType::HighwayMotorway, InOutCitySpeedKMpH(110.0 /* in city */, 120.0 /* out city */)},
    {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH(95.0 /* in city */, 105.0 /* out city */)},
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(85.0 /* in city */, 100.0 /* out city */)},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(70.0 /* in city */, 85.0 /* out city */)},

    // Tier 2:
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(65.0 /* in city */, 85.0 /* out city */)},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(55.0 /* in city */, 70.0 /* out city */)},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(60.0 /* in city */, 75.0 /* out city */)},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(48.0 /* in city */, 60.0 /* out city */)},

    // Tier 3:
    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(50.0 /* in city */, 65.0 /* out city */)},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH(40.0 /* in city */, 50.0 /* out city */)},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(35.0 /* in city */, 50.0 /* out city */)},

    // Tier 4:
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH({20.0, 28.0} /* in city */, {28.0, 28.0} /* out city */)},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(10.0 /* in city */, 10.0 /* out city */)},

    // Offroad & other:
    {HighwayType::HighwayService, InOutCitySpeedKMpH(15.0 /* in city */, 15.0 /* out city */)},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH(15.0 /* in city */, 20.0 /* out city */)},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH(25.0 /* in city */, 30.0 /* out city */)},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH({17.0, 10.0} /* in city */, {17.0, 10.0} /* out city */)},

    {HighwayType::RouteFerry, InOutCitySpeedKMpH(10.0 /* in city */, 10.0 /* out city */)},
    {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH(25.0 /* in city */, 25.0 /* out city */)},
};

// Offroad base speeds: boosted m_weight on HighwayTrack and HighwayRoad so the router actively
// prefers gravel/dirt detours (~2x+ longer) over paved roads, while m_eta stays realistic.
HighwayBasedSpeeds const kOffroadSpeeds = {
    {HighwayType::HighwayMotorway, InOutCitySpeedKMpH(110.0, 120.0)},
    {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH(95.0, 105.0)},
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(85.0, 100.0)},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(70.0, 85.0)},

    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(65.0, 85.0)},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(55.0, 70.0)},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(60.0, 75.0)},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(48.0, 60.0)},

    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(50.0, 65.0)},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH(40.0, 50.0)},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(35.0, 50.0)},

    {HighwayType::HighwayResidential, InOutCitySpeedKMpH({20.0, 28.0}, {28.0, 28.0})},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(10.0, 10.0)},

    {HighwayType::HighwayService, InOutCitySpeedKMpH(15.0, 15.0)},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH({45.0, 15.0}, {55.0, 20.0})},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH({80.0, 25.0}, {85.0, 30.0})},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH({17.0, 10.0}, {17.0, 10.0})},

    {HighwayType::RouteFerry, InOutCitySpeedKMpH(10.0, 10.0)},
    {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH(25.0, 25.0)},
};

// HardEnduro base speeds: aggressively prioritizes tracks/forest trails over asphalt,
// while keeping m_eta realistic and all speeds well below kMaxMotorcycleSpeedKMpH (200.0).
HighwayBasedSpeeds const kHardEnduroSpeeds = {
    {HighwayType::HighwayMotorway, InOutCitySpeedKMpH(110.0, 120.0)},
    {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH(95.0, 105.0)},
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(85.0, 100.0)},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(70.0, 85.0)},

    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(65.0, 85.0)},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(55.0, 70.0)},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(60.0, 75.0)},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(48.0, 60.0)},

    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(50.0, 65.0)},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH(40.0, 50.0)},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(35.0, 50.0)},

    {HighwayType::HighwayResidential, InOutCitySpeedKMpH({20.0, 28.0}, {28.0, 28.0})},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(10.0, 10.0)},

    {HighwayType::HighwayService, InOutCitySpeedKMpH(15.0, 15.0)},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH({60.0, 15.0}, {75.0, 20.0})},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH({110.0, 18.0}, {120.0, 20.0})},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH({17.0, 10.0}, {17.0, 10.0})},

    {HighwayType::RouteFerry, InOutCitySpeedKMpH(10.0, 10.0)},
    {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH(25.0, 25.0)},
};

// All SpeedFactor values stay in (0.0, 1.0]:
// - weightFactor = 1.0 / penalty (where penalty >= 1.0) preserves A* admissibility.
// - When m_eta < 1.0, m_weight is kept < 1.0 (e.g. 0.99) so SpeedKMpH::operator< stays valid at maxspeed=200.
// - etaFactor reflects real-world motorcycle travel speed and is kept consistent across profiles.

// Default (UI: Quick) profile: strongly prefers motorways and trunks even when slightly longer in km.
HighwayBasedFactors const kDefaultFactors = {
    {HighwayType::HighwayMotorway, InOutCityFactor(SpeedFactor{0.99 /* weight */, 0.93 /* eta */})},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(SpeedFactor{0.85, 0.75})},
    {HighwayType::HighwayTrunk, InOutCityFactor(SpeedFactor{0.95, 0.90})},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(SpeedFactor{0.85, 0.80})},

    {HighwayType::HighwayPrimary,
     InOutCityFactor(SpeedFactor{0.55, 0.80} /* in city */, SpeedFactor{0.60, 0.85} /* out city */)},
    {HighwayType::HighwayPrimaryLink,
     InOutCityFactor(SpeedFactor{0.50, 0.70} /* in city */, SpeedFactor{0.55, 0.75} /* out city */)},
    {HighwayType::HighwaySecondary,
     InOutCityFactor(SpeedFactor{0.42, 0.80} /* in city */, SpeedFactor{0.45, 0.85} /* out city */)},
    {HighwayType::HighwaySecondaryLink,
     InOutCityFactor(SpeedFactor{0.38, 0.70} /* in city */, SpeedFactor{0.40, 0.75} /* out city */)},

    {HighwayType::HighwayTertiary,
     InOutCityFactor(SpeedFactor{0.32, 0.70} /* in city */, SpeedFactor{0.35, 0.75} /* out city */)},
    {HighwayType::HighwayTertiaryLink,
     InOutCityFactor(SpeedFactor{0.28, 0.60} /* in city */, SpeedFactor{0.30, 0.65} /* out city */)},
    {HighwayType::HighwayUnclassified,
     InOutCityFactor(SpeedFactor{0.22, 0.70} /* in city */, SpeedFactor{0.25, 0.75} /* out city */)},

    {HighwayType::HighwayResidential, InOutCityFactor(SpeedFactor{0.40, 0.70})},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(SpeedFactor{0.40, 0.70})},
    {HighwayType::HighwayService, InOutCityFactor(SpeedFactor{0.40, 0.70})},
    {HighwayType::HighwayRoad, InOutCityFactor(SpeedFactor{0.45, 0.85})},
    {HighwayType::HighwayTrack, InOutCityFactor(SpeedFactor{0.20 /* weight */, 0.95 /* eta */})},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},

    {HighwayType::RouteFerry, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},
};

// Touring profile: comfortable smooth paved main roads (primary/secondary/trunk);
// penalizes motorways, rough asphalt, unclassified roads, and unpaved tracks.
HighwayBasedFactors const kTouringFactors = {
    {HighwayType::HighwayMotorway, InOutCityFactor(SpeedFactor{0.35 /* weight */, 0.93 /* eta */})},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(SpeedFactor{0.30, 0.75})},
    {HighwayType::HighwayTrunk, InOutCityFactor(SpeedFactor{0.95, 0.90})},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(SpeedFactor{0.85, 0.80})},

    {HighwayType::HighwayPrimary,
     InOutCityFactor(SpeedFactor{0.92, 0.80} /* in city */, SpeedFactor{0.98, 0.85} /* out city */)},
    {HighwayType::HighwayPrimaryLink,
     InOutCityFactor(SpeedFactor{0.80, 0.70} /* in city */, SpeedFactor{0.85, 0.75} /* out city */)},
    {HighwayType::HighwaySecondary,
     InOutCityFactor(SpeedFactor{0.90, 0.80} /* in city */, SpeedFactor{0.95, 0.85} /* out city */)},
    {HighwayType::HighwaySecondaryLink,
     InOutCityFactor(SpeedFactor{0.78, 0.70} /* in city */, SpeedFactor{0.82, 0.75} /* out city */)},

    {HighwayType::HighwayTertiary,
     InOutCityFactor(SpeedFactor{0.70, 0.70} /* in city */, SpeedFactor{0.75, 0.75} /* out city */)},
    {HighwayType::HighwayTertiaryLink,
     InOutCityFactor(SpeedFactor{0.60, 0.60} /* in city */, SpeedFactor{0.65, 0.65} /* out city */)},
    {HighwayType::HighwayUnclassified,
     InOutCityFactor(SpeedFactor{0.50, 0.70} /* in city */, SpeedFactor{0.55, 0.75} /* out city */)},

    {HighwayType::HighwayResidential, InOutCityFactor(SpeedFactor{0.55, 0.70})},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(SpeedFactor{0.45, 0.70})},
    {HighwayType::HighwayService, InOutCityFactor(SpeedFactor{0.45, 0.70})},
    {HighwayType::HighwayRoad, InOutCityFactor(SpeedFactor{0.50, 0.85})},
    {HighwayType::HighwayTrack, InOutCityFactor(SpeedFactor{0.15 /* weight */, 0.95 /* eta */})},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},

    {HighwayType::RouteFerry, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},
};

// Fun profile: ~3x penalty on motorways/trunks; prefers twisty secondary/tertiary/unclassified roads.
HighwayBasedFactors const kFunFactors = {
    {HighwayType::HighwayMotorway, InOutCityFactor(SpeedFactor{0.33 /* weight */, 0.93 /* eta */})},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(SpeedFactor{0.30, 0.75})},
    {HighwayType::HighwayTrunk, InOutCityFactor(SpeedFactor{0.33, 0.90})},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(SpeedFactor{0.30, 0.80})},

    {HighwayType::HighwayPrimary,
     InOutCityFactor(SpeedFactor{0.65, 0.80} /* in city */, SpeedFactor{0.70, 0.85} /* out city */)},
    {HighwayType::HighwayPrimaryLink,
     InOutCityFactor(SpeedFactor{0.60, 0.70} /* in city */, SpeedFactor{0.65, 0.75} /* out city */)},
    {HighwayType::HighwaySecondary,
     InOutCityFactor(SpeedFactor{0.85, 0.80} /* in city */, SpeedFactor{0.95, 0.85} /* out city */)},
    {HighwayType::HighwaySecondaryLink,
     InOutCityFactor(SpeedFactor{0.75, 0.70} /* in city */, SpeedFactor{0.80, 0.75} /* out city */)},

    {HighwayType::HighwayTertiary,
     InOutCityFactor(SpeedFactor{0.90, 0.70} /* in city */, SpeedFactor{0.95, 0.75} /* out city */)},
    {HighwayType::HighwayTertiaryLink,
     InOutCityFactor(SpeedFactor{0.75, 0.60} /* in city */, SpeedFactor{0.80, 0.65} /* out city */)},
    {HighwayType::HighwayUnclassified,
     InOutCityFactor(SpeedFactor{0.90, 0.70} /* in city */, SpeedFactor{0.95, 0.75} /* out city */)},

    {HighwayType::HighwayResidential, InOutCityFactor(SpeedFactor{0.50, 0.70})},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(SpeedFactor{0.40, 0.70})},
    {HighwayType::HighwayService, InOutCityFactor(SpeedFactor{0.40, 0.70})},
    {HighwayType::HighwayRoad, InOutCityFactor(SpeedFactor{0.60, 0.85})},
    {HighwayType::HighwayTrack, InOutCityFactor(SpeedFactor{0.25, 0.95})},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},

    {HighwayType::RouteFerry, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},
};

// Offroad profile: strongly favors tracks/gravel over asphalt so it happily takes ~2x gravel detours.
HighwayBasedFactors const kOffroadFactors = {
    {HighwayType::HighwayMotorway, InOutCityFactor(SpeedFactor{0.15 /* weight */, 0.93 /* eta */})},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(SpeedFactor{0.15, 0.75})},
    {HighwayType::HighwayTrunk, InOutCityFactor(SpeedFactor{0.25, 0.90})},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(SpeedFactor{0.22, 0.80})},

    {HighwayType::HighwayPrimary,
     InOutCityFactor(SpeedFactor{0.40, 0.80} /* in city */, SpeedFactor{0.42, 0.85} /* out city */)},
    {HighwayType::HighwayPrimaryLink,
     InOutCityFactor(SpeedFactor{0.35, 0.70} /* in city */, SpeedFactor{0.38, 0.75} /* out city */)},
    {HighwayType::HighwaySecondary,
     InOutCityFactor(SpeedFactor{0.50, 0.80} /* in city */, SpeedFactor{0.55, 0.85} /* out city */)},
    {HighwayType::HighwaySecondaryLink,
     InOutCityFactor(SpeedFactor{0.45, 0.70} /* in city */, SpeedFactor{0.50, 0.75} /* out city */)},

    {HighwayType::HighwayTertiary,
     InOutCityFactor(SpeedFactor{0.68, 0.70} /* in city */, SpeedFactor{0.72, 0.75} /* out city */)},
    {HighwayType::HighwayTertiaryLink,
     InOutCityFactor(SpeedFactor{0.60, 0.60} /* in city */, SpeedFactor{0.65, 0.65} /* out city */)},
    {HighwayType::HighwayUnclassified,
     InOutCityFactor(SpeedFactor{0.85, 0.70} /* in city */, SpeedFactor{0.90, 0.75} /* out city */)},

    {HighwayType::HighwayResidential, InOutCityFactor(SpeedFactor{0.50, 0.70})},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(SpeedFactor{0.45, 0.70})},
    {HighwayType::HighwayService, InOutCityFactor(SpeedFactor{0.60, 0.70})},
    {HighwayType::HighwayRoad, InOutCityFactor(SpeedFactor{0.90, 0.85})},
    {HighwayType::HighwayTrack, InOutCityFactor(1.0)},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},

    {HighwayType::RouteFerry, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},
};

// HardEnduro profile: aggressively flees asphalt onto forest/field tracks and rough unpaved roads.
HighwayBasedFactors const kHardEnduroFactors = {
    {HighwayType::HighwayMotorway, InOutCityFactor(SpeedFactor{0.08 /* weight */, 0.93 /* eta */})},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(SpeedFactor{0.08, 0.75})},
    {HighwayType::HighwayTrunk, InOutCityFactor(SpeedFactor{0.08, 0.90})},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(SpeedFactor{0.08, 0.80})},

    {HighwayType::HighwayPrimary,
     InOutCityFactor(SpeedFactor{0.25, 0.80} /* in city */, SpeedFactor{0.28, 0.85} /* out city */)},
    {HighwayType::HighwayPrimaryLink,
     InOutCityFactor(SpeedFactor{0.22, 0.70} /* in city */, SpeedFactor{0.25, 0.75} /* out city */)},
    {HighwayType::HighwaySecondary,
     InOutCityFactor(SpeedFactor{0.30, 0.80} /* in city */, SpeedFactor{0.30, 0.85} /* out city */)},
    {HighwayType::HighwaySecondaryLink,
     InOutCityFactor(SpeedFactor{0.28, 0.70} /* in city */, SpeedFactor{0.28, 0.75} /* out city */)},

    {HighwayType::HighwayTertiary,
     InOutCityFactor(SpeedFactor{0.45, 0.70} /* in city */, SpeedFactor{0.48, 0.75} /* out city */)},
    {HighwayType::HighwayTertiaryLink,
     InOutCityFactor(SpeedFactor{0.40, 0.60} /* in city */, SpeedFactor{0.42, 0.65} /* out city */)},
    {HighwayType::HighwayUnclassified,
     InOutCityFactor(SpeedFactor{0.70, 0.70} /* in city */, SpeedFactor{0.75, 0.75} /* out city */)},

    {HighwayType::HighwayResidential, InOutCityFactor(SpeedFactor{0.35, 0.70})},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(SpeedFactor{0.35, 0.70})},
    {HighwayType::HighwayService, InOutCityFactor(SpeedFactor{0.55, 0.70})},
    {HighwayType::HighwayRoad, InOutCityFactor(SpeedFactor{0.95, 0.85})},
    {HighwayType::HighwayTrack, InOutCityFactor(1.0)},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},

    {HighwayType::RouteFerry, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},
};

// Surface factors using valid OM classificator 'psurface=*' types.
// Note: VehicleModel clamps maxspeed-tagged roads using 'paved_bad' (m_minSurfaceFactorForMaxspeed),
// so paved_bad.m_weight is kept <= paved_good.m_weight for Offroad/HardEnduro.

VehicleModel::SurfaceInitList const kDefaultSurface = {
    // {{surfaceType}, {weightFactor, etaFactor}}
    {{"psurface", "paved_good"}, {1.0, 1.0}},
    {{"psurface", "paved_bad"}, {0.6, 0.7}},
    {{"psurface", "unpaved_good"}, {0.3, 0.7}},
    {{"psurface", "unpaved_bad"}, {0.15, 0.3}},
};

VehicleModel::SurfaceInitList const kTouringSurface = {
    {{"psurface", "paved_good"}, {1.0, 1.0}},
    {{"psurface", "paved_bad"}, {0.5, 0.75}},
    {{"psurface", "unpaved_good"}, {0.25, 0.65}},
    {{"psurface", "unpaved_bad"}, {0.1, 0.3}},
};

VehicleModel::SurfaceInitList const kFunSurface = {
    {{"psurface", "paved_good"}, {1.0, 1.0}},
    {{"psurface", "paved_bad"}, {0.65, 0.7}},
    {{"psurface", "unpaved_good"}, {0.35, 0.65}},
    {{"psurface", "unpaved_bad"}, {0.15, 0.3}},
};

VehicleModel::SurfaceInitList const kOffroadSurface = {
    {{"psurface", "paved_good"}, {0.45, 1.0}},  // >2.2x weight penalty on asphalt, realistic ETA
    {{"psurface", "paved_bad"}, {0.45, 0.8}},
    {{"psurface", "unpaved_good"}, {1.0, 0.8}},
    {{"psurface", "unpaved_bad"}, {0.85, 0.5}},
};

VehicleModel::SurfaceInitList const kHardEnduroSurface = {
    {{"psurface", "paved_good"}, {0.25, 1.0}},  // 4.0x weight penalty on asphalt, realistic ETA
    {{"psurface", "paved_bad"}, {0.25, 0.75}},
    {{"psurface", "unpaved_good"}, {1.0, 0.85}},
    {{"psurface", "unpaved_bad"}, {1.0, 0.55}},
};

HighwayBasedSpeeds const & GetSpeeds(MotorcycleProfile profile)
{
  switch (profile)
  {
  case MotorcycleProfile::Default:
  case MotorcycleProfile::Touring:
  case MotorcycleProfile::Fun: return kHighwayBasedSpeeds;
  case MotorcycleProfile::Offroad: return kOffroadSpeeds;
  case MotorcycleProfile::HardEnduro: return kHardEnduroSpeeds;
  }
  UNREACHABLE();
}

HighwayBasedFactors const & GetFactors(MotorcycleProfile profile)
{
  switch (profile)
  {
  case MotorcycleProfile::Default: return kDefaultFactors;
  case MotorcycleProfile::Fun: return kFunFactors;
  case MotorcycleProfile::Offroad: return kOffroadFactors;
  case MotorcycleProfile::HardEnduro: return kHardEnduroFactors;
  case MotorcycleProfile::Touring: return kTouringFactors;
  }
  UNREACHABLE();
}
}  // namespace motorcycle_model

namespace routing
{
std::string DebugPrint(MotorcycleProfile profile)
{
  switch (profile)
  {
  case MotorcycleProfile::Default: return "Default";
  case MotorcycleProfile::Fun: return "Fun";
  case MotorcycleProfile::Offroad: return "Offroad";
  case MotorcycleProfile::HardEnduro: return "HardEnduro";
  case MotorcycleProfile::Touring: return "Touring";
  }
  UNREACHABLE();
}

MotorcycleModel::MotorcycleModel(MotorcycleProfile profile) : MotorcycleModel(GetOptions(profile), profile) {}

MotorcycleModel::MotorcycleModel(LimitsInitList const & roadLimits, MotorcycleProfile profile)
  : VehicleModel(classif(), roadLimits, GetSurfaces(profile),
                 {motorcycle_model::GetSpeeds(profile), motorcycle_model::GetFactors(profile)})
  , m_profile(profile)
{
  auto const & speeds = motorcycle_model::GetSpeeds(profile);
  ASSERT_EQUAL(speeds.size(), motorcycle_model::GetFactors(profile).size(), ());
  ASSERT_EQUAL(speeds.size(), motorcycle_model::kDefaultOptions.size(), ());

  base::StringIL hwtagYesCar = {"hwtag", "yescar"};
  auto const & cl = classif();

  m_noType = cl.GetTypeByPath({"hwtag", "nocar"});
  m_yesType = cl.GetTypeByPath(hwtagYesCar);

  AddAdditionalRoadTypes(cl, {{hwtagYesCar, speeds.Get(HighwayType::HighwayTrack)}});

  // Set max possible (reasonable) motorcycle speed for admissible A* heuristic (EdgeEstimator::CalcHeuristic).
  SpeedKMpH constexpr kMaxMotorcycleSpeedKMpH(200.0);
  CHECK_LESS(m_maxModelSpeed, kMaxMotorcycleSpeedKMpH, ());
  m_maxModelSpeed = kMaxMotorcycleSpeedKMpH;
}

SpeedKMpH MotorcycleModel::GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const
{
  if ((m_profile == MotorcycleProfile::Offroad || m_profile == MotorcycleProfile::HardEnduro) &&
      speedParams.m_defSpeedKmPH != kInvalidSpeed)
  {
    auto const hwType = GetHighwayType(types);
    if (hwType == HighwayType::HighwayTrack || hwType == HighwayType::HighwayRoad)
    {
      // Preserve boosted offroad weight speeds on tracks/roads instead of letting MWM car default speeds
      // overwrite m_weight (and collapse m_eta) in VehicleModel::GetTypeSpeedImpl.
      SpeedParams params = speedParams;
      params.m_defSpeedKmPH = kInvalidSpeed;
      return GetTypeSpeedImpl(types, params, true /* isCar */);
    }
  }
  return GetTypeSpeedImpl(types, speedParams, true /* isCar */);
}

SpeedKMpH const & MotorcycleModel::GetOffroadSpeed() const
{
  return motorcycle_model::kSpeedOffroadKMpH;
}

// static
MotorcycleModel const & MotorcycleModel::AllLimitsInstance(MotorcycleProfile profile)
{
  switch (profile)
  {
  case MotorcycleProfile::Default:
  {
    static MotorcycleModel const instance(MotorcycleProfile::Default);
    return instance;
  }
  case MotorcycleProfile::Fun:
  {
    static MotorcycleModel const instance(MotorcycleProfile::Fun);
    return instance;
  }
  case MotorcycleProfile::Offroad:
  {
    static MotorcycleModel const instance(MotorcycleProfile::Offroad);
    return instance;
  }
  case MotorcycleProfile::HardEnduro:
  {
    static MotorcycleModel const instance(MotorcycleProfile::HardEnduro);
    return instance;
  }
  case MotorcycleProfile::Touring:
  {
    static MotorcycleModel const instance(MotorcycleProfile::Touring);
    return instance;
  }
  }
  UNREACHABLE();
}

// static
VehicleModel::LimitsInitList const & MotorcycleModel::GetOptions(MotorcycleProfile /* profile */)
{
  return motorcycle_model::kDefaultOptions;
}

// static
VehicleModel::SurfaceInitList const & MotorcycleModel::GetSurfaces(MotorcycleProfile profile)
{
  switch (profile)
  {
  case MotorcycleProfile::Default: return motorcycle_model::kDefaultSurface;
  case MotorcycleProfile::Fun: return motorcycle_model::kFunSurface;
  case MotorcycleProfile::Offroad: return motorcycle_model::kOffroadSurface;
  case MotorcycleProfile::HardEnduro: return motorcycle_model::kHardEnduroSurface;
  case MotorcycleProfile::Touring: return motorcycle_model::kTouringSurface;
  }
  UNREACHABLE();
}

MotorcycleModelFactory::MotorcycleModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn,
                                               MotorcycleProfile profile)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  m_models[""] = std::make_shared<MotorcycleModel>(profile);
}
}  // namespace routing
