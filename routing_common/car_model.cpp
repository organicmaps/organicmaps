#include "routing_common/car_model.hpp"

#include "base/macros.hpp"

#include "indexer/classificator.hpp"

#include <algorithm>
#include <vector>

using namespace std;
using namespace routing;

namespace
{
using InOutCitySpeedKMpH = VehicleModel::InOutCitySpeedKMpH;
using SpeedKMpH = VehicleModel::SpeedKMpH;
using MaxspeedFactor = VehicleModel::MaxspeedFactor;

// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway
// Speed of road features located inside and outside cities and towns polygons in km per hour.
//                                                          in city            out city
InOutCitySpeedKMpH constexpr             kSpeedMotorwayKMpH(SpeedKMpH(117.8),  SpeedKMpH(123.4), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr         kSpeedMotorwayLinkKMpH(SpeedKMpH(82.0),   SpeedKMpH(81.2),  MaxspeedFactor(0.85));
InOutCitySpeedKMpH constexpr                kSpeedTrunkKMpH(SpeedKMpH(83.4),   SpeedKMpH(100.2), MaxspeedFactor(1.0));
InOutCitySpeedKMpH constexpr            kSpeedTrunkLinkKMpH(SpeedKMpH(73.0),   SpeedKMpH(77.2),  MaxspeedFactor(0.85));
InOutCitySpeedKMpH constexpr              kSpeedPrimaryKMpH(SpeedKMpH(63.1),   SpeedKMpH(75.2),  MaxspeedFactor(0.95));
InOutCitySpeedKMpH constexpr          kSpeedPrimaryLinkKMpH(SpeedKMpH(66.5),   SpeedKMpH(64.8),  MaxspeedFactor(0.8));
InOutCitySpeedKMpH constexpr            kSpeedSecondaryKMpH(SpeedKMpH(52.8),   SpeedKMpH(60.3),  MaxspeedFactor(0.9));
InOutCitySpeedKMpH constexpr        kSpeedSecondaryLinkKMpH(SpeedKMpH(50.2),   SpeedKMpH(60.0),  MaxspeedFactor(0.75));
InOutCitySpeedKMpH constexpr             kSpeedTertiaryKMpH(SpeedKMpH(45.5),   SpeedKMpH(50.5),  MaxspeedFactor(0.85));
InOutCitySpeedKMpH constexpr         kSpeedTertiaryLinkKMpH(SpeedKMpH(25.0),   SpeedKMpH(30.0),  MaxspeedFactor(0.7));
InOutCitySpeedKMpH constexpr          kSpeedResidentialKMpH(SpeedKMpH(20.0),   SpeedKMpH(25.0),  MaxspeedFactor(0.75));
InOutCitySpeedKMpH constexpr         kSpeedUnclassifiedKMpH(SpeedKMpH(51.3),   SpeedKMpH(66.0),  MaxspeedFactor(0.8));
InOutCitySpeedKMpH constexpr              kSpeedServiceKMpH(SpeedKMpH(15.0),   SpeedKMpH(15.0),  MaxspeedFactor(0.8));
InOutCitySpeedKMpH constexpr         kSpeedLivingStreetKMpH(SpeedKMpH(10.0),   SpeedKMpH(10.0),  MaxspeedFactor(0.75));
InOutCitySpeedKMpH constexpr                 kSpeedRoadKMpH(SpeedKMpH(10.0),   SpeedKMpH(10.0),  MaxspeedFactor(0.3));
InOutCitySpeedKMpH constexpr                kSpeedTrackKMpH(SpeedKMpH(5.0),    SpeedKMpH(5.0),   MaxspeedFactor(0.3));
InOutCitySpeedKMpH constexpr        kSpeedFerryMotorcarKMpH(SpeedKMpH(10.0),   SpeedKMpH(10.0),  MaxspeedFactor(0.9));
InOutCitySpeedKMpH constexpr kSpeedFerryMotorcarVehicleKMpH(SpeedKMpH(10.0),   SpeedKMpH(10.0),  MaxspeedFactor(0.9));
InOutCitySpeedKMpH constexpr  kSpeedRailMotorcarVehicleKMpH(SpeedKMpH(10.0),   SpeedKMpH(10.0),  MaxspeedFactor(0.9));
InOutCitySpeedKMpH constexpr         kSpeedShuttleTrainKMpH(SpeedKMpH(25.0),   SpeedKMpH(25.0),  MaxspeedFactor(0.9));
InOutCitySpeedKMpH constexpr                 kSpeedPierKMpH(SpeedKMpH(10.0),   SpeedKMpH(10.0),  MaxspeedFactor(0.9));

double constexpr kSpeedOffroadKMpH = 10.0;

VehicleModel::LimitsInitList const g_carLimitsDefault =
{
  // {{roadType, roadType}        Speed km per hour    passThroughAllowed}
  {{"highway", "motorway"},       kSpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kSpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true}
  /// @todo: Add to classificator
  //{ {"highway", "shuttle_train"},  10 },
  //{ {"highway", "ferry"},          5  },
  //{ {"highway", "default"},        10 },
  /// @todo: Check type
  //{ {"highway", "construction"},   40 },
};

VehicleModel::LimitsInitList const g_carLimitsNoPassThroughLivingStreet =
{
  {{"highway", "motorway"},       kSpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kSpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  false},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true}
};

VehicleModel::LimitsInitList const g_carLimitsNoPassThroughLivingStreetAndService =
{
  {{"highway", "motorway"},       kSpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kSpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       false},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  false},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         true}
};

VehicleModel::LimitsInitList const g_carLimitsAustralia = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsAustria = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsBelarus = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsBelgium = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsBrazil = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsDenmark =
{
  // No track
  {{"highway", "motorway"},       kSpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kSpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true}
};

VehicleModel::LimitsInitList const g_carLimitsFrance = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsFinland = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsGermany =
{
  // No pass through track
  {{"highway", "motorway"},       kSpeedMotorwayKMpH,      true},
  {{"highway", "motorway_link"},  kSpeedMotorwayLinkKMpH,  true},
  {{"highway", "trunk"},          kSpeedTrunkKMpH,         true},
  {{"highway", "trunk_link"},     kSpeedTrunkLinkKMpH,     true},
  {{"highway", "primary"},        kSpeedPrimaryKMpH,       true},
  {{"highway", "primary_link"},   kSpeedPrimaryLinkKMpH,   true},
  {{"highway", "secondary"},      kSpeedSecondaryKMpH,     true},
  {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
  {{"highway", "tertiary"},       kSpeedTertiaryKMpH,      true},
  {{"highway", "tertiary_link"},  kSpeedTertiaryLinkKMpH,  true},
  {{"highway", "residential"},    kSpeedResidentialKMpH,   true},
  {{"highway", "unclassified"},   kSpeedUnclassifiedKMpH,  true},
  {{"highway", "service"},        kSpeedServiceKMpH,       true},
  {{"highway", "living_street"},  kSpeedLivingStreetKMpH,  true},
  {{"highway", "road"},           kSpeedRoadKMpH,          true},
  {{"highway", "track"},          kSpeedTrackKMpH,         false}
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
  {{"route", "ferry", "motorcar"}, kSpeedFerryMotorcarKMpH},
  {{"route", "ferry", "motor_vehicle"}, kSpeedFerryMotorcarVehicleKMpH},
  {{"railway", "rail", "motor_vehicle"}, kSpeedRailMotorcarVehicleKMpH},
  {{"route", "shuttle_train"}, kSpeedShuttleTrainKMpH},
  {{"route", "ferry"}, kSpeedFerryMotorcarKMpH},
  {{"man_made", "pier"}, kSpeedPierKMpH}
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

double CarModel::GetOffroadSpeed() const { return kSpeedOffroadKMpH; }

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

// static
VehicleModel::SurfaceInitList const & CarModel::GetSurfaces() { return g_carSurface; }

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
