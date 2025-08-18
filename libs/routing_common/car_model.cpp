#include "routing_common/car_model.hpp"
#include "routing_common/car_model_coefs.hpp"

#include "indexer/classificator.hpp"

namespace car_model
{
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

VehicleModel::LimitsInitList const kDefaultOptions = {
    // {HighwayType, passThroughAllowed}
    {HighwayType::HighwayMotorway, true},    {HighwayType::HighwayMotorwayLink, true},
    {HighwayType::HighwayTrunk, true},       {HighwayType::HighwayTrunkLink, true},
    {HighwayType::HighwayPrimary, true},     {HighwayType::HighwayPrimaryLink, true},
    {HighwayType::HighwaySecondary, true},   {HighwayType::HighwaySecondaryLink, true},
    {HighwayType::HighwayTertiary, true},    {HighwayType::HighwayTertiaryLink, true},
    {HighwayType::HighwayResidential, true}, {HighwayType::HighwayUnclassified, true},
    {HighwayType::HighwayService, true},     {HighwayType::HighwayLivingStreet, true},
    {HighwayType::HighwayRoad, true},        {HighwayType::HighwayTrack, true},
    {HighwayType::RouteShuttleTrain, true},  {HighwayType::RouteFerry, true},
    {HighwayType::ManMadePier, true}};

VehicleModel::LimitsInitList NoPassThroughLivingStreet()
{
  auto res = kDefaultOptions;
  for (auto & e : res)
    if (e.m_type == HighwayType::HighwayLivingStreet)
      e.m_isPassThroughAllowed = false;
  return res;
}

VehicleModel::LimitsInitList NoPassThroughService(VehicleModel::LimitsInitList res = kDefaultOptions)
{
  for (auto & e : res)
    if (e.m_type == HighwayType::HighwayService)
      e.m_isPassThroughAllowed = false;
  return res;
}

VehicleModel::LimitsInitList NoTrack()
{
  VehicleModel::LimitsInitList res;
  res.reserve(kDefaultOptions.size() - 1);
  for (auto const & e : kDefaultOptions)
    if (e.m_type != HighwayType::HighwayTrack)
      res.push_back(e);
  return res;
}

VehicleModel::LimitsInitList NoPassThroughTrack()
{
  auto res = kDefaultOptions;
  for (auto & e : res)
    if (e.m_type == HighwayType::HighwayTrack)
      e.m_isPassThroughAllowed = false;
  return res;
}

/// @todo Should make some compare constrains (like in CarModel_TrackVsGravelTertiary test)
/// to better fit these factors with reality. I have no idea, how they were set.
VehicleModel::SurfaceInitList const kCarSurface = {
    // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
    {{"psurface", "paved_good"}, {1.0, 1.0}},
    {{"psurface", "paved_bad"}, {0.6, 0.7}},
    {{"psurface", "unpaved_good"}, {0.4, 0.7}},
    {{"psurface", "unpaved_bad"}, {0.2, 0.3}}};
}  // namespace car_model

namespace routing
{
CarModel::CarModel() : CarModel(car_model::kDefaultOptions) {}

CarModel::CarModel(VehicleModel::LimitsInitList const & roadLimits)
  : VehicleModel(classif(), roadLimits, car_model::kCarSurface, {kHighwayBasedSpeeds, kHighwayBasedFactors})
{
  ASSERT_EQUAL(kHighwayBasedSpeeds.size(), kHighwayBasedFactors.size(), ());
  ASSERT_EQUAL(kHighwayBasedSpeeds.size(), car_model::kDefaultOptions.size(), ());

  std::vector<std::string> hwtagYesCar = {"hwtag", "yescar"};
  auto const & cl = classif();

  m_noType = cl.GetTypeByPath({"hwtag", "nocar"});
  m_yesType = cl.GetTypeByPath(hwtagYesCar);

  // Set small track speed if highway is not in kHighwayBasedSpeeds (path, pedestrian), but marked as yescar.
  AddAdditionalRoadTypes(cl, {{std::move(hwtagYesCar), kHighwayBasedSpeeds.Get(HighwayType::HighwayTrack)}});

  // Set max possible (reasonable) car speed. See EdgeEstimator::CalcHeuristic.
  SpeedKMpH constexpr kMaxCarSpeedKMpH(200.0);
  CHECK_LESS(m_maxModelSpeed, kMaxCarSpeedKMpH, ());
  m_maxModelSpeed = kMaxCarSpeedKMpH;
}

SpeedKMpH CarModel::GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const
{
  return GetTypeSpeedImpl(types, speedParams, true /* isCar */);
}

SpeedKMpH const & CarModel::GetOffroadSpeed() const
{
  return car_model::kSpeedOffroadKMpH;
}

// static
CarModel const & CarModel::AllLimitsInstance()
{
  static CarModel const instance;
  return instance;
}

// static
VehicleModel::LimitsInitList const & CarModel::GetOptions()
{
  return car_model::kDefaultOptions;
}

// static
VehicleModel::SurfaceInitList const & CarModel::GetSurfaces()
{
  return car_model::kCarSurface;
}

CarModelFactory::CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  using namespace car_model;
  using std::make_shared;

  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<CarModel>();

  m_models["Austria"] = make_shared<CarModel>(NoPassThroughLivingStreet());
  m_models["Belarus"] = make_shared<CarModel>(NoPassThroughLivingStreet());
  m_models["Brazil"] = make_shared<CarModel>(NoPassThroughService(NoPassThroughTrack()));
  m_models["Denmark"] = make_shared<CarModel>(NoTrack());
  m_models["Germany"] = make_shared<CarModel>(NoPassThroughTrack());
  m_models["Hungary"] = make_shared<CarModel>(NoPassThroughLivingStreet());
  m_models["Poland"] = make_shared<CarModel>(NoPassThroughService());
  m_models["Romania"] = make_shared<CarModel>(NoPassThroughLivingStreet());
  m_models["Russian Federation"] = make_shared<CarModel>(NoPassThroughService(NoPassThroughLivingStreet()));
  m_models["Slovakia"] = make_shared<CarModel>(NoPassThroughLivingStreet());
  m_models["Ukraine"] = make_shared<CarModel>(NoPassThroughService(NoPassThroughLivingStreet()));
}
}  // namespace routing
