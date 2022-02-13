#include "routing_common/car_model.hpp"
#include "routing_common/car_model_coefs.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/macros.hpp"

#include <array>
#include <unordered_map>

namespace car_model
{
using namespace std;
using namespace routing;

// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

// |kSpeedOffroadKMpH| is a speed which is used for edges that don't lie on road features.
// For example for pure fake edges. In car routing, off road speed for calculation ETA is not used.
// The weight of such edges is considered as 0 seconds. It's especially actual when an airport is
// a start or finish. On the other hand, while route calculation the fake edges are considered
// as quite heavy. The idea behind that is to use the closest edge for the start and the finish
// of the route except for some edge cases.
SpeedKMpH constexpr kSpeedOffroadKMpH = {0.01 /* weight */, kNotUsed /* eta */};

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

VehicleModel::AdditionalRoadsList const kAdditionalRoads = {
    // {{highway tags}, {weightSpeed, etaSpeed}}
    {{"railway", "rail", "motor_vehicle"}, kHighwayBasedSpeeds.Get(HighwayType::RailwayRailMotorVehicle)},
    {{"route", "shuttle_train"}, kHighwayBasedSpeeds.Get(HighwayType::RouteShuttleTrain)},
    {{"route", "ferry"}, kHighwayBasedSpeeds.Get(HighwayType::RouteFerry)},
    {{"man_made", "pier"}, kHighwayBasedSpeeds.Get(HighwayType::ManMadePier)}};

VehicleModel::SurfaceInitList const kCarSurface = {
  // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
  {{"psurface", "paved_good"}, {1.0, 1.0}},
  {{"psurface", "paved_bad"}, {0.5, 0.5}},
  {{"psurface", "unpaved_good"}, {0.4, 0.8}},
  {{"psurface", "unpaved_bad"}, {0.1, 0.3}}
};

// Names must be the same with country names from countries.txt
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
}  // namespace car_model

namespace routing
{
CarModel::CarModel()
  : VehicleModel(classif(), car_model::kCarOptionsDefault, car_model::kCarSurface,
                 {kHighwayBasedSpeeds, kHighwayBasedFactors})
{
  Init();
}

CarModel::CarModel(VehicleModel::LimitsInitList const & roadLimits, HighwayBasedInfo const & info)
  : VehicleModel(classif(), roadLimits, car_model::kCarSurface, info)
{
  Init();
}

SpeedKMpH const & CarModel::GetOffroadSpeed() const { return car_model::kSpeedOffroadKMpH; }

void CarModel::Init()
{
  m_noCarType = classif().GetTypeByPath({"hwtag", "nocar"});
  m_yesCarType = classif().GetTypeByPath({"hwtag", "yescar"});

  AddAdditionalRoadTypes(classif(), car_model::kAdditionalRoads);
}

VehicleModelInterface::RoadAvailability CarModel::GetRoadAvailability(feature::TypesHolder const & types) const
{
  if (types.Has(m_yesCarType))
    return RoadAvailability::Available;

  if (types.Has(m_noCarType))
    return RoadAvailability::NotAvailable;

  return RoadAvailability::Unknown;
}

// static
CarModel const & CarModel::AllLimitsInstance()
{
  static CarModel const instance;
  return instance;
}

// static
VehicleModel::LimitsInitList const & CarModel::GetOptions() { return car_model::kCarOptionsDefault; }

// static
VehicleModel::AdditionalRoadsList const & CarModel::GetAdditionalRoads()
{
  return car_model::kAdditionalRoads;
}

// static
VehicleModel::SurfaceInitList const & CarModel::GetSurfaces() { return car_model::kCarSurface; }

CarModelFactory::CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  m_models[""] = std::make_shared<CarModel>(
      car_model::kCarOptionsDefault,
      HighwayBasedInfo(kHighwayBasedSpeeds, kHighwayBasedFactors));

  for (auto const & kv : car_model::kCarOptionsByCountries)
  {
    auto const * country = kv.first;
    auto const & limit = kv.second;
    m_models[country] =
        std::make_shared<CarModel>(limit, HighwayBasedInfo(kHighwayBasedSpeeds, kHighwayBasedFactors));
  }
}
}  // namespace routing
