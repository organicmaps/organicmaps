#include "car_model.hpp"

#include "base/macros.hpp"

#include "indexer/classificator.hpp"

#include "std/vector.hpp"

namespace
{
double constexpr kSpeedMotorwayKMpH = 90.0;
double constexpr kSpeedMotorwayLinkKMpH = 75.0;
double constexpr kSpeedTrunkKMpH = 85.0;
double constexpr kSpeedTrunkLinkKMpH = 70.0;
double constexpr kSpeedPrimaryKMpH = 65.0;
double constexpr kSpeedPrimaryLinkKMpH = 60.0;
double constexpr kSpeedSecondaryKMpH = 55.0;
double constexpr kSpeedSecondaryLinkKMpH = 50.0;
double constexpr kSpeedTertiaryKMpH = 40.0;
double constexpr kSpeedTertiaryLinkKMpH = 30.0;
double constexpr kSpeedResidentialKMpH = 25.0;
double constexpr kSpeedPedestrianKMpH = 25.0;
double constexpr kSpeedUnclassifiedKMpH = 25.0;
double constexpr kSpeedServiceKMpH = 15.0;
double constexpr kSpeedLivingStreetKMpH = 10.0;
double constexpr kSpeedRoadKMpH = 10.0;
double constexpr kSpeedTrackKMpH = 5.0;
double constexpr kSpeedFerryMotorcarKMpH = 15.0;
double constexpr kSpeedFerryMotorcarVehicleKMpH = 15.0;
double constexpr kSpeedRailMotorcarVehicleKMpH = 25.0;
double constexpr kSpeedShuttleTrainKMpH = 25.0;

routing::VehicleModel::InitListT const s_carLimits = {
    {{"highway", "motorway"}, kSpeedMotorwayKMpH},
    {{"highway", "trunk"}, kSpeedTrunkKMpH},
    {{"highway", "motorway_link"}, kSpeedMotorwayLinkKMpH},
    {{"highway", "trunk_link"}, kSpeedTrunkLinkKMpH},
    {{"highway", "primary"}, kSpeedPrimaryKMpH},
    {{"highway", "primary_link"}, kSpeedPrimaryLinkKMpH},
    {{"highway", "secondary"}, kSpeedSecondaryKMpH},
    {{"highway", "secondary_link"}, kSpeedSecondaryLinkKMpH},
    {{"highway", "tertiary"}, kSpeedTertiaryKMpH},
    {{"highway", "tertiary_link"}, kSpeedTertiaryLinkKMpH},
    {{"highway", "residential"}, kSpeedResidentialKMpH},
    {{"highway", "pedestrian"}, kSpeedPedestrianKMpH},
    {{"highway", "unclassified"}, kSpeedUnclassifiedKMpH},
    {{"highway", "service"}, kSpeedServiceKMpH},
    {{"highway", "living_street"}, kSpeedLivingStreetKMpH},
    {{"highway", "road"}, kSpeedRoadKMpH},
    {{"highway", "track"}, kSpeedTrackKMpH},
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

CarModelFactory::CarModelFactory()
{
  m_model = make_shared<CarModel>();
}

shared_ptr<IVehicleModel> CarModelFactory::GetVehicleModel() const
{
  return m_model;
}

shared_ptr<IVehicleModel> CarModelFactory::GetVehicleModelForCountry(string const & /* country */) const
{
  return m_model;
}
}  // namespace routing
