#include "routing_common/car_model.hpp"
#include "routing_common/car_model_coefs.hpp"

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
SpeedKMpH constexpr kOffroadSpeed = {0.01 /* weight */, kNotUsed /* eta */};

NoPassThroughHighways NoPassThroughLivingStreet()
{
  return {HighwayType::HighwayLivingStreet};
}

NoPassThroughHighways NoPassThroughLivingStreetAndService()
{
  return {HighwayType::HighwayLivingStreet, HighwayType::HighwayService};
}

HighwaySpeeds NoTrack()
{
  HighwaySpeeds res = kHighwayBasedSpeeds;
  res.erase(HighwayType::HighwayTrack);
  return res;
}

NoPassThroughHighways NoPassThroughTrack()
{
  return {HighwayType::HighwayTrack};
}

}  // namespace car_model

namespace routing
{

CarModel::CarModel(HighwaySpeeds const & speeds, NoPassThroughHighways const & noPassThrough)
  : VehicleModel(speeds, kHighwayBasedFactors, kHighwayBasedSurface, noPassThrough)
{
  auto const & cl = classif();
  m_noCarType = cl.GetTypeByPath({"hwtag", "nocar"});
  m_yesCarType = cl.GetTypeByPath({"hwtag", "yescar"});
  m_onewayType = cl.GetTypeByPath({"hwtag", "oneway"});
}

SpeedKMpH CarModel::GetOffroadSpeed() const
{
  return car_model::kOffroadSpeed;
}

VehicleModel::ResultT CarModel::IsOneWay(uint32_t type) const
{
  if (type == m_onewayType)
    return ResultT::Yes;
  return ResultT::Unknown;
}

VehicleModel::ResultT CarModel::GetRoadAvailability(uint32_t type) const
{
  if (type == m_yesCarType)
    return ResultT::Yes;
  else if (type == m_noCarType)
    return ResultT::No;
  return ResultT::Unknown;
}

SpeedKMpH CarModel::GetSpeedForAvailable() const
{
  /// @todo Return 20% from maximun model speed.
  return m_maxModelSpeed.m_inCity * 0.2;
}

// static
CarModel const & CarModel::AllLimitsInstance()
{
  static CarModel const instance(kHighwayBasedSpeeds);
  return instance;
}

CarModelFactory::CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  using namespace car_model;
  // Names must be the same with country names from countries.txt

  m_models[""].reset(new CarModel(kHighwayBasedSpeeds));
  m_models["Austria"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreet()));
  m_models["Belarus"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreet()));
  m_models["Denmark"].reset(new CarModel(NoTrack()));
  m_models["Germany"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughTrack()));
  m_models["Hungary"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreet()));
  m_models["Romania"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreet()));
  m_models["Russian Federation"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreetAndService()));
  m_models["Slovakia"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreet()));
  m_models["Ukraine"].reset(new CarModel(kHighwayBasedSpeeds, NoPassThroughLivingStreetAndService()));
}

}  // namespace routing
