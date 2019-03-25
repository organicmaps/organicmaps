#include "routing_common/car_model.hpp"
#include "routing_common/car_model_coefs.hpp"

#include "base/macros.hpp"

#include "indexer/classificator.hpp"

#include <array>
#include <unordered_map>

using namespace std;
using namespace routing;

namespace
{
// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

//  // Names must be the same with country names from countries.txt
std::array<char const *, 41> constexpr kCountries = {"Australia",
                                                     "Austria",
                                                     "Belarus",
                                                     "Belgium",
                                                     "Brazil",
                                                     "Canada",
                                                     "Colombia",
                                                     "Czech Republic",
                                                     "Denmark",
                                                     "Ecuador",
                                                     "Finland",
                                                     "France",
                                                     "Germany",
                                                     "Hungary",
                                                     "Indonesia",
                                                     "Ireland",
                                                     "Italy",
                                                     "Kuwait",
                                                     "Luxembourg",
                                                     "Mexico",
                                                     "Netherlands",
                                                     "New Zealand",
                                                     "Norway",
                                                     "Poland",
                                                     "Portugal",
                                                     "Romania",
                                                     "Russian Federation",
                                                     "Saudi Arabia",
                                                     "Singapore",
                                                     "Slovakia",
                                                     "South Africa",
                                                     "Spain",
                                                     "Sweden",
                                                     "Switzerland",
                                                     "Thailand",
                                                     "Turkey",
                                                     "Ukraine",
                                                     "United Arab Emirates",
                                                     "United Kingdom",
                                                     "United States of America",
                                                     "Venezuela"};

double constexpr kSpeedOffroadKMpH = 10.0;

VehicleModel::LimitsInitList const kCarOptionsDefault = {
    // {{roadType, roadType}  passThroughAllowed}
    {{"highway", "motorway"}, true},
    {{"highway", "motorway_link"}, true},
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "service"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true}
    /// @todo: Add to classificator
    //{ {"highway", "shuttle_train"},  10 },
    //{ {"highway", "ferry"},          5  },
    //{ {"highway", "default"},        10 },
    /// @todo: Check type
    //{ {"highway", "construction"},   40 },
};

VehicleModel::LimitsInitList const kCarOptionsNoPassThroughLivingStreet = {
    {{"highway", "motorway"}, true},
    {{"highway", "motorway_link"}, true},
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "service"}, true},
    {{"highway", "living_street"}, false},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true}};

VehicleModel::LimitsInitList const kCarOptionsNoPassThroughLivingStreetAndService = {
    {{"highway", "motorway"}, true},
    {{"highway", "motorway_link"}, true},
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "service"}, false},
    {{"highway", "living_street"}, false},
    {{"highway", "road"}, true},
    {{"highway", "track"}, true}};

VehicleModel::LimitsInitList const kCarOptionsDenmark = {
    // No track
    {{"highway", "motorway"}, true},
    {{"highway", "motorway_link"}, true},
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "service"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "road"}, true}};

VehicleModel::LimitsInitList const kCarOptionsGermany = {
    // No pass through track
    {{"highway", "motorway"}, true},
    {{"highway", "motorway_link"}, true},
    {{"highway", "trunk"}, true},
    {{"highway", "trunk_link"}, true},
    {{"highway", "primary"}, true},
    {{"highway", "primary_link"}, true},
    {{"highway", "secondary"}, true},
    {{"highway", "secondary_link"}, true},
    {{"highway", "tertiary"}, true},
    {{"highway", "tertiary_link"}, true},
    {{"highway", "residential"}, true},
    {{"highway", "unclassified"}, true},
    {{"highway", "service"}, true},
    {{"highway", "living_street"}, true},
    {{"highway", "road"}, true},
    {{"highway", "track"}, false}};

vector<VehicleModel::AdditionalRoadTags> const kAdditionalTags = {
    // {{highway tags}, {weightSpeed, etaSpeed}}
    {{"route", "ferry", "motorcar"}, kGlobalHighwayBasedMeanSpeeds.at(HighwayType::RouteFerryMotorcar)},
    {{"route", "ferry", "motor_vehicle"}, kGlobalHighwayBasedMeanSpeeds.at(HighwayType::RouteFerryMotorVehicle)},
    {{"railway", "rail", "motor_vehicle"}, kGlobalHighwayBasedMeanSpeeds.at(HighwayType::RailwayRailMotorVehicle)},
    {{"route", "shuttle_train"}, kGlobalHighwayBasedMeanSpeeds.at(HighwayType::RouteShuttleTrain)},
    {{"route", "ferry"}, kGlobalHighwayBasedMeanSpeeds.at(HighwayType::RouteFerryMotorcar)},
    {{"man_made", "pier"}, kGlobalHighwayBasedMeanSpeeds.at(HighwayType::ManMadePier)}};

VehicleModel::SurfaceInitList const kCarSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {0.5, 0.5}},
  {{"psurface", "unpaved_good"}, {0.8, 0.8}},
  {{"psurface", "unpaved_bad"}, {0.3, 0.3}}
};

std::unordered_map<char const *, VehicleModel::LimitsInitList> const kCarOptionsByCountries = {
    {"Austria", kCarOptionsNoPassThroughLivingStreet},
    {"Belarus", kCarOptionsNoPassThroughLivingStreet},
    {"Denmark", kCarOptionsDenmark},
    {"Germany", kCarOptionsGermany},
    {"Hungary", kCarOptionsNoPassThroughLivingStreet},
    {"Romania", kCarOptionsNoPassThroughLivingStreet},
    {"Russian Federation", kCarOptionsNoPassThroughLivingStreetAndService},
    {"Slovakia", kCarOptionsNoPassThroughLivingStreet},
    {"Ukraine", kCarOptionsNoPassThroughLivingStreetAndService}
};
}  // namespace

namespace routing
{
CarModel::CarModel()
  : VehicleModel(classif(), kCarOptionsDefault, kCarSurface,
                 {kGlobalHighwayBasedMeanSpeeds, kGlobalHighwayBasedFactors})
{
  InitAdditionalRoadTypes();
}

CarModel::CarModel(VehicleModel::LimitsInitList const & roadLimits, HighwayBasedInfo const & info)
  : VehicleModel(classif(), roadLimits, kCarSurface, info)
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
routing::VehicleModel::LimitsInitList const & CarModel::GetOptions() { return kCarOptionsDefault; }

// static
vector<routing::VehicleModel::AdditionalRoadTags> const & CarModel::GetAdditionalTags()
{
  return kAdditionalTags;
}

// static
VehicleModel::SurfaceInitList const & CarModel::GetSurfaces() { return kCarSurface; }

CarModelFactory::CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  auto const & speeds = kCountryToHighwayBasedMeanSpeeds;
  auto const & factors = kCountryToHighwayBasedFactors;
  m_models[""] = make_shared<CarModel>(
      kCarOptionsDefault,
      HighwayBasedInfo(kGlobalHighwayBasedMeanSpeeds, kGlobalHighwayBasedFactors));
  for (auto const * country : kCountries)
  {
    auto const limitIt = kCarOptionsByCountries.find(country);
    auto const & limit = limitIt == kCarOptionsByCountries.cend() ? kCarOptionsDefault : limitIt->second;
    auto const speedIt = speeds.find(country);
    auto const & speed = speedIt == speeds.cend() ? kGlobalHighwayBasedMeanSpeeds : speedIt->second;
    auto const factorIt = factors.find(country);
    auto const & factor =
        factorIt == factors.cend() ? kGlobalHighwayBasedFactors : factorIt->second;
    m_models[country] = make_shared<CarModel>(
        limit,
        HighwayBasedInfo(speed, kGlobalHighwayBasedMeanSpeeds, factor, kGlobalHighwayBasedFactors));
  }
}
}  // namespace routing
