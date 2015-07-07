#include "pedestrian_model.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

namespace
{

// See model specifics in different countries here:
// http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

// See road types here - http://wiki.openstreetmap.org/wiki/Key:highway

static double const kPedestrianSpeedVerySlowKMpH = 1;
static double const kPedestrianSpeedSlowKMpH = 2;
static double const kPedestrianSpeedBelowNormalKMpH = 4;
static double const kPedestrianSpeedNormalKMpH = 5;

routing::VehicleModel::InitListT const s_defaultPedestrianLimits =
{
  { {"highway", "motorway"},       kPedestrianSpeedVerySlowKMpH },
  { {"highway", "trunk"},          kPedestrianSpeedVerySlowKMpH },
  { {"highway", "motorway_link"},  kPedestrianSpeedVerySlowKMpH },
  { {"highway", "trunk_link"},     kPedestrianSpeedVerySlowKMpH },
  { {"highway", "primary"},        kPedestrianSpeedSlowKMpH },
  { {"highway", "primary_link"},   kPedestrianSpeedSlowKMpH },
  { {"highway", "secondary"},      kPedestrianSpeedSlowKMpH },
  { {"highway", "secondary_link"}, kPedestrianSpeedSlowKMpH },
  { {"highway", "tertiary"},       kPedestrianSpeedSlowKMpH },
  { {"highway", "tertiary_link"},  kPedestrianSpeedSlowKMpH },
  { {"highway", "service"},        kPedestrianSpeedSlowKMpH },
  { {"highway", "unclassified"},   kPedestrianSpeedBelowNormalKMpH },
  { {"highway", "road"},           kPedestrianSpeedBelowNormalKMpH },
  { {"highway", "track"},          kPedestrianSpeedBelowNormalKMpH },
  { {"highway", "path"},           kPedestrianSpeedBelowNormalKMpH },
  { {"highway", "residential"},    kPedestrianSpeedNormalKMpH },
  { {"highway", "living_street"},  kPedestrianSpeedNormalKMpH },
  { {"highway", "steps"},          kPedestrianSpeedNormalKMpH },
  { {"highway", "pedestrian"},     kPedestrianSpeedNormalKMpH },
  { {"highway", "footway"},        kPedestrianSpeedNormalKMpH },
  // all other are restricted
};

}  // namespace

namespace routing
{

PedestrianModel::PedestrianModel()
  : VehicleModel(classif(), s_defaultPedestrianLimits)
{
  Init();
}

PedestrianModel::PedestrianModel(VehicleModel::InitListT const & speedLimits)
  : VehicleModel(classif(), speedLimits)
{
  Init();
}

void PedestrianModel::Init()
{
  m_noFootType = classif().GetTypeByPath({ "hwtag", "nofoot" });

  initializer_list<char const *> arr[] =
  {
    { "route", "ferry" },
    { "man_made", "pier" },
  };

  SetAdditionalRoadTypes(classif(), arr, ARRAY_SIZE(arr));
}

bool PedestrianModel::IsFoot(feature::TypesHolder const & types) const
{
  return find(types.begin(), types.end(), m_noFootType) == types.end();
}

double PedestrianModel::GetSpeed(FeatureType const & f) const
{
  feature::TypesHolder types(f);

  if (IsFoot(types) && IsRoad(types))
    return VehicleModel::GetSpeed(types);

  return 0.0;
}


PedestrianModelFactory::PedestrianModelFactory()
{
  m_models[string()] = make_shared<PedestrianModel>();
}

shared_ptr<IVehicleModel> PedestrianModelFactory::GetVehicleModel() const
{
  auto const itr = m_models.find(string());
  ASSERT(itr != m_models.end(), ("Default VehicleModel must be specified"));
  return itr->second;
}

shared_ptr<IVehicleModel> PedestrianModelFactory::GetVehicleModelForCountry(string const & country) const
{
  auto const itr = m_models.find(country);
  if (itr != m_models.end())
    return itr->second;
  return PedestrianModelFactory::GetVehicleModel();
}

}  // routing
