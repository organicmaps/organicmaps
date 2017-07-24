#include "routing_common/car_model.hpp"

#include "base/macros.hpp"

#include "indexer/classificator.hpp"

#include "std/vector.hpp"

namespace
{
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

routing::VehicleModel::InitListT const s_carLimits = {
    {{"highway", "motorway"}, kSpeedMotorwayKMpH,            true},
    {{"highway", "trunk"}, kSpeedTrunkKMpH,                  true},
    {{"highway", "motorway_link"}, kSpeedMotorwayLinkKMpH,   true},
    {{"highway", "trunk_link"}, kSpeedTrunkLinkKMpH,         true},
    {{"highway", "primary"}, kSpeedPrimaryKMpH,              true},
    {{"highway", "primary_link"}, kSpeedPrimaryLinkKMpH,     true},
    {{"highway", "secondary"}, kSpeedSecondaryKMpH,          true},
    {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH, true},
    {{"highway", "tertiary"}, kSpeedTertiaryKMpH,            true},
    {{"highway", "tertiary_link"}, kSpeedTertiaryLinkKMpH,   true},
    {{"highway", "residential"}, kSpeedResidentialKMpH,      true},
    {{"highway", "unclassified"}, kSpeedUnclassifiedKMpH,    true},
    {{"highway", "service"}, kSpeedServiceKMpH,              true},
    {{"highway", "living_street"}, kSpeedLivingStreetKMpH,   true},
    {{"highway", "road"}, kSpeedRoadKMpH,                    true},
    {{"highway", "track"}, kSpeedTrackKMpH,                  true},
    /// @todo: Add to classificator
    //{ {"highway", "shuttle_train"},  10 },
    //{ {"highway", "ferry"},          5  },
    //{ {"highway", "default"},        10 },
    /// @todo: Check type
    //{ {"highway", "construction"},   40 },
};

}  // namespace

namespace routing
{

CarModel::CarModel()
  : VehicleModel(classif(), s_carLimits)
{
  vector<AdditionalRoadTags> const additionalTags = {
      {{"route", "ferry", "motorcar"}, kSpeedFerryMotorcarKMpH},
      {{"route", "ferry", "motor_vehicle"}, kSpeedFerryMotorcarVehicleKMpH},
      {{"railway", "rail", "motor_vehicle"}, kSpeedRailMotorcarVehicleKMpH},
      {{"route", "shuttle_train"}, kSpeedShuttleTrainKMpH},
  };

  SetAdditionalRoadTypes(classif(), additionalTags);
}

// static
CarModel const & CarModel::AllLimitsInstance()
{
  static CarModel const instance;
  return instance;
}

CarModelFactory::CarModelFactory() { m_model = make_shared<CarModel>(); }
shared_ptr<IVehicleModel> CarModelFactory::GetVehicleModel() const { return m_model; }
shared_ptr<IVehicleModel> CarModelFactory::GetVehicleModelForCountry(
    string const & /* country */) const
{
  // @TODO(bykoianko) Different vehicle model for different country should be supported
  // according to http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions.
  // See pedestrian_model.cpp and bicycle_model.cpp for example.
  return m_model;
}
}  // namespace routing
