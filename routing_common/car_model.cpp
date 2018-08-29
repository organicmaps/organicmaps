#include "routing_common/car_model.hpp"

#include "base/macros.hpp"

#include "indexer/classificator.hpp"

#include <vector>

using namespace std;
using namespace routing;

namespace
{
// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

// Speed of road features located outside cities and towns polygons in km per hour.
double constexpr kOutCitySpeedMotorwayKMpH = 115.37;
double constexpr kOutCitySpeedMotorwayLinkKMpH = 75.0;
double constexpr kOutCitySpeedTrunkKMpH = 93.89;
double constexpr kOutCitySpeedTrunkLinkKMpH = 70.0;
double constexpr kOutCitySpeedPrimaryKMpH = 84.29;
double constexpr kOutCitySpeedPrimaryLinkKMpH = 60.0;
double constexpr kOutCitySpeedSecondaryKMpH = 72.23;
double constexpr kOutCitySpeedSecondaryLinkKMpH = 50.0;
double constexpr kOutCitySpeedTertiaryKMpH = 62.63;
double constexpr kOutCitySpeedTertiaryLinkKMpH = 30.0;
double constexpr kOutCitySpeedResidentialKMpH = 25.0;
double constexpr kOutCitySpeedUnclassifiedKMpH = 51.09;
double constexpr kOutCitySpeedServiceKMpH = 15.0;
double constexpr kOutCitySpeedLivingStreetKMpH = 10.0;
double constexpr kOutCitySpeedRoadKMpH = 10.0;
double constexpr kOutCitySpeedTrackKMpH = 5.0;
double constexpr kOutCitySpeedFerryMotorcarKMpH = 15.0;
double constexpr kOutCitySpeedFerryMotorcarVehicleKMpH = 15.0;
double constexpr kOutCitySpeedRailMotorcarVehicleKMpH = 25.0;
double constexpr kOutCitySpeedShuttleTrainKMpH = 25.0;
double constexpr kOutCitySpeedPierKMpH = 10.0;
double constexpr kOutCitySpeedOffroadKMpH = 10.0;

// Speed of road features located inside cities and towns polygons in km per hour.
double constexpr kInCitySpeedMotorwayKMpH = kOutCitySpeedMotorwayKMpH;
double constexpr kInCitySpeedMotorwayLinkKMpH = kOutCitySpeedMotorwayLinkKMpH;
double constexpr kInCitySpeedTrunkKMpH = 70.0;
double constexpr kInCitySpeedTrunkLinkKMpH = 50.0;
double constexpr kInCitySpeedPrimaryKMpH = 65.0;
double constexpr kInCitySpeedPrimaryLinkKMpH = 55.0;
double constexpr kInCitySpeedSecondaryKMpH = 45.0;
double constexpr kInCitySpeedSecondaryLinkKMpH = 40.0;
double constexpr kInCitySpeedTertiaryKMpH = 40.0;
double constexpr kInCitySpeedTertiaryLinkKMpH = 30.0;
double constexpr kInCitySpeedResidentialKMpH = 25.0;
double constexpr kInCitySpeedUnclassifiedKMpH = 25.0;
double constexpr kInCitySpeedServiceKMpH = kOutCitySpeedServiceKMpH;
double constexpr kInCitySpeedLivingStreetKMpH = kOutCitySpeedLivingStreetKMpH;
double constexpr kInCitySpeedRoadKMpH = kOutCitySpeedRoadKMpH;
double constexpr kInCitySpeedTrackKMpH = kOutCitySpeedTrackKMpH;
double constexpr kInCitySpeedFerryMotorcarKMpH = kOutCitySpeedFerryMotorcarKMpH;
double constexpr kInCitySpeedFerryMotorcarVehicleKMpH = kOutCitySpeedFerryMotorcarVehicleKMpH;
double constexpr kInCitySpeedRailMotorcarVehicleKMpH = kOutCitySpeedRailMotorcarVehicleKMpH;
double constexpr kInCitySpeedShuttleTrainKMpH = kOutCitySpeedShuttleTrainKMpH;
double constexpr kInCitySpeedPierKMpH = kOutCitySpeedPierKMpH;
double constexpr kInCitySpeedOffroadKMpH = kOutCitySpeedOffroadKMpH;

VehicleModel::LimitsInitList const g_carLimitsDefault =
{
  // {{roadType, roadType}        {weightSpeedKMpH, etSpeedKMpH}    passThroughAllowed}
  {{"highway", "motorway"},       kOutCitySpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kOutCitySpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true}
  /// @todo: Add to classificator
  //{ {"highway", "shuttle_train"},  10 },
  //{ {"highway", "ferry"},          5  },
  //{ {"highway", "default"},        10 },
  /// @todo: Check type
  //{ {"highway", "construction"},   40 },
};

VehicleModel::LimitsInitList const g_carLimitsNoPassThroughLivingStreet =
{
  {{"highway", "motorway"},       kOutCitySpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kOutCitySpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  false},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true}
};

VehicleModel::LimitsInitList const g_carLimitsNoPassThroughLivingStreetAndService =
{
  {{"highway", "motorway"},       kOutCitySpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kOutCitySpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       false},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  false},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         true}
};

VehicleModel::LimitsInitList const g_carLimitsAustralia = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsAustria = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsBelarus = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsBelgium = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsBrazil = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsDenmark =
{
  // No track
  {{"highway", "motorway"},       kOutCitySpeedMotorwayKMpH,       true},
  {{"highway", "motorway_link"},  kOutCitySpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true}
};

VehicleModel::LimitsInitList const g_carLimitsFrance = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsFinland = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsGermany =
{
  // No pass through track
  {{"highway", "motorway"},       kOutCitySpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kOutCitySpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kOutCitySpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kOutCitySpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kOutCitySpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kOutCitySpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kOutCitySpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kOutCitySpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kOutCitySpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kOutCitySpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kOutCitySpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kOutCitySpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kOutCitySpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kOutCitySpeedLivingStreetKMpH,  true},
  {{"highway", "road"},           kOutCitySpeedRoadKMpH,          true},
  {{"highway", "track"},          kOutCitySpeedTrackKMpH,         false}
};

VehicleModel::LimitsInitList const g_carLimitsHungary = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsIceland = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsNetherlands = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsNorway = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsOman = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsPoland = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsRomania = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsRussia = g_carLimitsNoPassThroughLivingStreetAndService;

VehicleModel::LimitsInitList const g_carLimitsSlovakia = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsSpain = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsSwitzerland = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsTurkey = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsUkraine = g_carLimitsNoPassThroughLivingStreetAndService;

VehicleModel::LimitsInitList const g_carLimitsUK = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsUS = g_carLimitsDefault;

vector<VehicleModel::AdditionalRoadTags> const kAdditionalTags = {
  // {{highway tags}, {weightSpeed, etaSpeed}}
  {{"route", "ferry", "motorcar"}, kOutCitySpeedFerryMotorcarKMpH},
  {{"route", "ferry", "motor_vehicle"}, kOutCitySpeedFerryMotorcarVehicleKMpH},
  {{"railway", "rail", "motor_vehicle"}, kOutCitySpeedRailMotorcarVehicleKMpH},
  {{"route", "shuttle_train"}, kOutCitySpeedShuttleTrainKMpH},
  {{"route", "ferry"}, kOutCitySpeedFerryMotorcarKMpH},
  {{"man_made", "pier"}, kOutCitySpeedPierKMpH}
};

VehicleModel::SurfaceInitList const g_carSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {0.5, 0.5}},
  {{"psurface", "unpaved_good"}, {0.8, 0.8}},
  {{"psurface", "unpaved_bad"}, {0.3, 0.3}}
};
}  // namespace

namespace routing
{

CarModel::CarModel()
  : VehicleModel(classif(), g_carLimitsDefault, g_carSurface)
{
  InitAdditionalRoadTypes();
}

CarModel::CarModel(VehicleModel::LimitsInitList const & roadLimits)
  : VehicleModel(classif(), roadLimits, g_carSurface)
{
  InitAdditionalRoadTypes();
}

double CarModel::GetOffroadSpeed() const { return kOutCitySpeedOffroadKMpH; }

void CarModel::InitAdditionalRoadTypes()
{
  SetAdditionalRoadTypes(classif(), kAdditionalTags);
}

// static
CarModel const & CarModel::AllLimitsInstance()
{
  static CarModel const instance;
  return instance;
}

// static
routing::VehicleModel::LimitsInitList const & CarModel::GetLimits() { return g_carLimitsDefault; }
// static
vector<routing::VehicleModel::AdditionalRoadTags> const & CarModel::GetAdditionalTags()
{
  return kAdditionalTags;
}

CarModelFactory::CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<CarModel>(g_carLimitsDefault);
  m_models["Australia"] = make_shared<CarModel>(g_carLimitsAustralia);
  m_models["Austria"] = make_shared<CarModel>(g_carLimitsAustria);
  m_models["Belarus"] = make_shared<CarModel>(g_carLimitsBelarus);
  m_models["Belgium"] = make_shared<CarModel>(g_carLimitsBelgium);
  m_models["Brazil"] = make_shared<CarModel>(g_carLimitsBrazil);
  m_models["Denmark"] = make_shared<CarModel>(g_carLimitsDenmark);
  m_models["France"] = make_shared<CarModel>(g_carLimitsFrance);
  m_models["Finland"] = make_shared<CarModel>(g_carLimitsFinland);
  m_models["Germany"] = make_shared<CarModel>(g_carLimitsGermany);
  m_models["Hungary"] = make_shared<CarModel>(g_carLimitsHungary);
  m_models["Iceland"] = make_shared<CarModel>(g_carLimitsIceland);
  m_models["Netherlands"] = make_shared<CarModel>(g_carLimitsNetherlands);
  m_models["Norway"] = make_shared<CarModel>(g_carLimitsNorway);
  m_models["Oman"] = make_shared<CarModel>(g_carLimitsOman);
  m_models["Poland"] = make_shared<CarModel>(g_carLimitsPoland);
  m_models["Romania"] = make_shared<CarModel>(g_carLimitsRomania);
  m_models["Russian Federation"] = make_shared<CarModel>(g_carLimitsRussia);
  m_models["Slovakia"] = make_shared<CarModel>(g_carLimitsSlovakia);
  m_models["Spain"] = make_shared<CarModel>(g_carLimitsSpain);
  m_models["Switzerland"] = make_shared<CarModel>(g_carLimitsSwitzerland);
  m_models["Turkey"] = make_shared<CarModel>(g_carLimitsTurkey);
  m_models["Ukraine"] = make_shared<CarModel>(g_carLimitsUkraine);
  m_models["United Kingdom"] = make_shared<CarModel>(g_carLimitsUK);
  m_models["United States of America"] = make_shared<CarModel>(g_carLimitsUS);
}
}  // namespace routing
