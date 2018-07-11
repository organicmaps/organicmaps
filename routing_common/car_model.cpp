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

double constexpr kSpeedMotorwayKMpH = 115.37;
double constexpr kSpeedMotorwayLinkKMpH = 75.0;
double constexpr kSpeedTrunkKMpH = 93.89;
double constexpr kSpeedTrunkLinkKMpH = 70.0;
double constexpr kSpeedPrimaryKMpH = 84.29;
double constexpr kSpeedPrimaryLinkKMpH = 60.0;
double constexpr kSpeedSecondaryKMpH = 72.23;
double constexpr kSpeedSecondaryLinkKMpH = 50.0;
double constexpr kSpeedTertiaryKMpH = 62.63;
double constexpr kSpeedTertiaryLinkKMpH = 30.0;
double constexpr kSpeedResidentialKMpH = 25.0;
double constexpr kSpeedUnclassifiedKMpH = 51.09;
double constexpr kSpeedServiceKMpH = 15.0;
double constexpr kSpeedLivingStreetKMpH = 10.0;
double constexpr kSpeedRoadKMpH = 10.0;
double constexpr kSpeedTrackKMpH = 5.0;
double constexpr kSpeedFerryMotorcarKMpH = 15.0;
double constexpr kSpeedFerryMotorcarVehicleKMpH = 15.0;
double constexpr kSpeedRailMotorcarVehicleKMpH = 25.0;
double constexpr kSpeedShuttleTrainKMpH = 25.0;
double constexpr kSpeedPierKMpH = 10.0;
double constexpr kSpeedOffroadKMpH = 10.0;

VehicleModel::LimitsInitList const g_carLimitsDefault =
{
  // {{roadType, roadType}        {weightSpeedKMpH,         etSpeedKMpH}              passThroughAllowed}
  {{"highway", "motorway"},       {kSpeedMotorwayKMpH,      kSpeedMotorwayKMpH},      true},
  {{"highway", "motorway_link"},  {kSpeedMotorwayLinkKMpH,  kSpeedMotorwayLinkKMpH},  true},
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true}
  /// @todo: Add to classificator
  //{ {"highway", "shuttle_train"},  10 },
  //{ {"highway", "ferry"},          5  },
  //{ {"highway", "default"},        10 },
  /// @todo: Check type
  //{ {"highway", "construction"},   40 },
};

VehicleModel::LimitsInitList const g_carLimitsNoPassThroughLivingStreet =
{
  {{"highway", "motorway"},       {kSpeedMotorwayKMpH,      kSpeedMotorwayKMpH},      true},
  {{"highway", "motorway_link"},  {kSpeedMotorwayLinkKMpH,  kSpeedMotorwayLinkKMpH},  true},
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  false},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true}
};

VehicleModel::LimitsInitList const g_carLimitsNoPassThroughLivingStreetAndService =
{
  {{"highway", "motorway"},       {kSpeedMotorwayKMpH,      kSpeedMotorwayKMpH},      true},
  {{"highway", "motorway_link"},  {kSpeedMotorwayLinkKMpH,  kSpeedMotorwayLinkKMpH},  true},
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       false},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  false},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         true}
};

VehicleModel::LimitsInitList const g_carLimitsAustralia = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsAustria = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsBelarus = g_carLimitsNoPassThroughLivingStreet;

VehicleModel::LimitsInitList const g_carLimitsBelgium = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsBrazil = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsDenmark =
{
  // No track
  {{"highway", "motorway"},       {kSpeedMotorwayKMpH,      kSpeedMotorwayKMpH},      true},
  {{"highway", "motorway_link"},  {kSpeedMotorwayLinkKMpH,  kSpeedMotorwayLinkKMpH},  true},
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedPrimaryLinkKMpH},   true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true}
};

VehicleModel::LimitsInitList const g_carLimitsFrance = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsFinland = g_carLimitsDefault;

VehicleModel::LimitsInitList const g_carLimitsGermany =
{
  // No pass through track
  {{"highway", "motorway"},       {kSpeedMotorwayKMpH,      kSpeedMotorwayKMpH},      true},
  {{"highway", "motorway_link"},  {kSpeedMotorwayLinkKMpH,  kSpeedMotorwayLinkKMpH},  true},
  {{"highway", "trunk"},          {kSpeedTrunkKMpH,         kSpeedTrunkKMpH},         true},
  {{"highway", "trunk_link"},     {kSpeedTrunkLinkKMpH,     kSpeedTrunkLinkKMpH},     true},
  {{"highway", "primary"},        {kSpeedPrimaryKMpH,       kSpeedPrimaryKMpH},       true},
  {{"highway", "primary_link"},   {kSpeedPrimaryLinkKMpH,   kSpeedMotorwayLinkKMpH},  true},
  {{"highway", "secondary"},      {kSpeedSecondaryKMpH,     kSpeedSecondaryKMpH},     true},
  {{"highway", "secondary_link"}, {kSpeedSecondaryLinkKMpH, kSpeedSecondaryLinkKMpH}, true},
  {{"highway", "tertiary"},       {kSpeedTertiaryKMpH,      kSpeedTertiaryKMpH},      true},
  {{"highway", "tertiary_link"},  {kSpeedTertiaryLinkKMpH,  kSpeedTertiaryLinkKMpH},  true},
  {{"highway", "residential"},    {kSpeedResidentialKMpH,   kSpeedResidentialKMpH},   true},
  {{"highway", "unclassified"},   {kSpeedUnclassifiedKMpH,  kSpeedUnclassifiedKMpH},  true},
  {{"highway", "service"},        {kSpeedServiceKMpH,       kSpeedServiceKMpH},       true},
  {{"highway", "living_street"},  {kSpeedLivingStreetKMpH,  kSpeedLivingStreetKMpH},  true},
  {{"highway", "road"},           {kSpeedRoadKMpH,          kSpeedRoadKMpH},          true},
  {{"highway", "track"},          {kSpeedTrackKMpH,         kSpeedTrackKMpH},         false}
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
  {{"route", "ferry", "motorcar"}, {kSpeedFerryMotorcarKMpH, kSpeedFerryMotorcarKMpH}},
  {{"route", "ferry", "motor_vehicle"}, {kSpeedFerryMotorcarVehicleKMpH, kSpeedFerryMotorcarVehicleKMpH}},
  {{"railway", "rail", "motor_vehicle"}, {kSpeedRailMotorcarVehicleKMpH, kSpeedRailMotorcarVehicleKMpH}},
  {{"route", "shuttle_train"}, {kSpeedShuttleTrainKMpH, kSpeedShuttleTrainKMpH}},
  {{"route", "ferry"}, {kSpeedFerryMotorcarKMpH, kSpeedFerryMotorcarKMpH}},
  {{"man_made", "pier"}, {kSpeedPierKMpH, kSpeedPierKMpH}}
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
