#include "routing/features_road_graph.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/route.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

namespace routing
{

namespace
{
uint32_t constexpr kPowOfTwoForFeatureCacheSize = 10; // cache contains 2 ^ kPowOfTwoForFeatureCacheSize elements

double constexpr kMwmRoadCrossingRadiusMeters = 2.0;

double constexpr kMwmCrossingNodeEqualityRadiusMeters = 100.0;

}  // namespace

double GetRoadCrossingRadiusMeters() { return kMwmRoadCrossingRadiusMeters; }

FeaturesRoadGraph::Value::Value(Index const & index, MwmSet::MwmHandle handle)
  : m_mwmHandle(move(handle))
{
  if (!m_mwmHandle.IsAlive())
    return;

  m_altitudeLoader = make_unique<feature::AltitudeLoader>(index, m_mwmHandle.GetId());
}

FeaturesRoadGraph::CrossCountryVehicleModel::CrossCountryVehicleModel(
    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory)
  : m_vehicleModelFactory(vehicleModelFactory)
  , m_maxSpeedKMPH(m_vehicleModelFactory->GetVehicleModel()->GetMaxSpeed())
  , m_offroadSpeedKMPH(m_vehicleModelFactory->GetVehicleModel()->GetOffroadSpeed())
{
}

double FeaturesRoadGraph::CrossCountryVehicleModel::GetSpeed(FeatureType const & f) const
{
  return GetVehicleModel(f.GetID())->GetSpeed(f);
}

double FeaturesRoadGraph::CrossCountryVehicleModel::GetMaxSpeed() const
{
  return m_maxSpeedKMPH;
}

double FeaturesRoadGraph::CrossCountryVehicleModel::GetOffroadSpeed() const
{
  return m_offroadSpeedKMPH;
}

bool FeaturesRoadGraph::CrossCountryVehicleModel::IsOneWay(FeatureType const & f) const
{
  return GetVehicleModel(f.GetID())->IsOneWay(f);
}

bool FeaturesRoadGraph::CrossCountryVehicleModel::IsRoad(FeatureType const & f) const
{
  return GetVehicleModel(f.GetID())->IsRoad(f);
}

bool FeaturesRoadGraph::CrossCountryVehicleModel::IsPassThroughAllowed(FeatureType const & f) const
{
  return GetVehicleModel(f.GetID())->IsPassThroughAllowed(f);
}

VehicleModelInterface * FeaturesRoadGraph::CrossCountryVehicleModel::GetVehicleModel(FeatureID const & featureId) const
{
  auto itr = m_cache.find(featureId.m_mwmId);
  if (itr != m_cache.end())
    return itr->second.get();

  auto const vehicleModel = m_vehicleModelFactory->GetVehicleModelForCountry(
      featureId.m_mwmId.GetInfo()->GetCountryName());

  ASSERT(nullptr != vehicleModel, ());
  ASSERT_EQUAL(m_maxSpeedKMPH, vehicleModel->GetMaxSpeed(), ());

  itr = m_cache.insert(make_pair(featureId.m_mwmId, move(vehicleModel))).first;
  return itr->second.get();
}

void FeaturesRoadGraph::CrossCountryVehicleModel::Clear()
{
  m_cache.clear();
}


IRoadGraph::RoadInfo & FeaturesRoadGraph::RoadInfoCache::Find(FeatureID const & featureId, bool & found)
{
  auto res = m_cache.insert(make_pair(featureId.m_mwmId, TMwmFeatureCache()));
  if (res.second)
    res.first->second.Init(kPowOfTwoForFeatureCacheSize);
  return res.first->second.Find(featureId.m_index, found);
}

void FeaturesRoadGraph::RoadInfoCache::Clear()
{
  m_cache.clear();
}
FeaturesRoadGraph::FeaturesRoadGraph(Index const & index, IRoadGraph::Mode mode,
                                     shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory)
  : m_index(index), m_mode(mode), m_vehicleModel(vehicleModelFactory)
{
}

int FeaturesRoadGraph::GetStreetReadScale() { return scales::GetUpperScale(); }

class CrossFeaturesLoader
{
public:
  CrossFeaturesLoader(FeaturesRoadGraph const & graph, IRoadGraph::ICrossEdgesLoader & edgesLoader)
    : m_graph(graph), m_edgesLoader(edgesLoader)
  {}

  void operator()(FeatureType & ft)
  {
    if (!m_graph.IsRoad(ft))
      return;

    double const speedKMPH = m_graph.GetSpeedKMPHFromFt(ft);
    if (speedKMPH <= 0.0)
      return;

    FeatureID const featureId = ft.GetID();

    IRoadGraph::RoadInfo const & roadInfo = m_graph.GetCachedRoadInfo(featureId, ft, speedKMPH);

    m_edgesLoader(featureId, roadInfo);
  }

private:
  FeaturesRoadGraph const & m_graph;
  IRoadGraph::ICrossEdgesLoader & m_edgesLoader;
};

IRoadGraph::RoadInfo FeaturesRoadGraph::GetRoadInfo(FeatureID const & featureId) const
{
  RoadInfo const & ri = GetCachedRoadInfo(featureId);
  ASSERT_GREATER(ri.m_speedKMPH, 0.0, ());
  return ri;
}

double FeaturesRoadGraph::GetSpeedKMPH(FeatureID const & featureId) const
{
  double const speedKMPH = GetCachedRoadInfo(featureId).m_speedKMPH;
  ASSERT_GREATER(speedKMPH, 0.0, ());
  return speedKMPH;
}

double FeaturesRoadGraph::GetMaxSpeedKMPH() const
{
  return m_vehicleModel.GetMaxSpeed();
}

void FeaturesRoadGraph::ForEachFeatureClosestToCross(m2::PointD const & cross,
                                                     ICrossEdgesLoader & edgesLoader) const
{
  CrossFeaturesLoader featuresLoader(*this, edgesLoader);
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(cross, kMwmRoadCrossingRadiusMeters);
  m_index.ForEachInRect(featuresLoader, rect, GetStreetReadScale());
}

void FeaturesRoadGraph::FindClosestEdges(m2::PointD const & point, uint32_t count,
                                         vector<pair<Edge, Junction>> & vicinities) const
{
  NearestEdgeFinder finder(point);

  auto const f = [&finder, this](FeatureType & ft)
  {
    if (!m_vehicleModel.IsRoad(ft))
      return;

    double const speedKMPH = m_vehicleModel.GetSpeed(ft);
    if (speedKMPH <= 0.0)
      return;

    FeatureID const featureId = ft.GetID();

    IRoadGraph::RoadInfo const & roadInfo = GetCachedRoadInfo(featureId, ft, speedKMPH);

    finder.AddInformationSource(featureId, roadInfo);
  };

  m_index.ForEachInRect(
      f, MercatorBounds::RectByCenterXYAndSizeInMeters(point, kMwmCrossingNodeEqualityRadiusMeters),
      GetStreetReadScale());

  finder.MakeResult(vicinities, count);
}

void FeaturesRoadGraph::GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const
{
  FeatureType ft;
  Index::FeaturesLoaderGuard loader(m_index, featureId.m_mwmId);
  if (!loader.GetFeatureByIndex(featureId.m_index, ft))
    return;

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());
  types = feature::TypesHolder(ft);
}

void FeaturesRoadGraph::GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const
{
  types = feature::TypesHolder();

  m2::PointD const & cross = junction.GetPoint();

  auto const f = [&types, &cross](FeatureType const & ft)
  {
    if (!types.Empty())
      return;

    if (ft.GetFeatureType() != feature::GEOM_POINT)
      return;

    if (!my::AlmostEqualAbs(ft.GetCenter(), cross, routing::kPointsEqualEpsilon))
      return;

    feature::TypesHolder typesHolder(ft);
    if (!typesHolder.Empty())
      types = typesHolder;
  };

  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(cross, kMwmRoadCrossingRadiusMeters);
  m_index.ForEachInRect(f, rect, GetStreetReadScale());
}

IRoadGraph::Mode FeaturesRoadGraph::GetMode() const
{
  return m_mode;
};

void FeaturesRoadGraph::ClearState()
{
  m_cache.Clear();
  m_vehicleModel.Clear();
  m_mwmLocks.clear();
}

bool FeaturesRoadGraph::IsRoad(FeatureType const & ft) const { return m_vehicleModel.IsRoad(ft); }

bool FeaturesRoadGraph::IsOneWay(FeatureType const & ft) const
{
  return m_vehicleModel.IsOneWay(ft);
}

double FeaturesRoadGraph::GetSpeedKMPHFromFt(FeatureType const & ft) const
{
  return m_vehicleModel.GetSpeed(ft);
}

void FeaturesRoadGraph::ExtractRoadInfo(FeatureID const & featureId, FeatureType const & ft,
                                        double speedKMPH, RoadInfo & ri) const
{
  Value const & value = LockMwm(featureId.m_mwmId);
  if (!value.IsAlive())
    return;

  ri.m_bidirectional = !IsOneWay(ft);
  ri.m_speedKMPH = speedKMPH;

  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
  size_t const pointsCount = ft.GetPointsCount();

  feature::TAltitudes altitudes;
  if (value.m_altitudeLoader)
  {
    altitudes = value.m_altitudeLoader->GetAltitudes(featureId.m_index, ft.GetPointsCount());
  }
  else
  {
    ASSERT(false, ());
    altitudes = feature::TAltitudes(ft.GetPointsCount(), feature::kDefaultAltitudeMeters);
  }

  CHECK_EQUAL(altitudes.size(), pointsCount,
              ("altitudeLoader->GetAltitudes(", featureId.m_index, "...) returns wrong alititudes:",
               altitudes));

  ri.m_junctions.resize(pointsCount);
  for (size_t i = 0; i < pointsCount; ++i)
    ri.m_junctions[i] = Junction(ft.GetPoint(i), altitudes[i]);
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(FeatureID const & featureId) const
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(featureId, found);

  if (found)
    return ri;

  FeatureType ft;

  Index::FeaturesLoaderGuard loader(m_index, featureId.m_mwmId);

  if (!loader.GetFeatureByIndex(featureId.m_index, ft))
    return ri;

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());

  ExtractRoadInfo(featureId, ft, GetSpeedKMPHFromFt(ft), ri);
  return ri;
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(FeatureID const & featureId,
                                                                  FeatureType const & ft,
                                                                  double speedKMPH) const
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(featureId, found);

  if (found)
    return ri;

  // ft must be set
  ASSERT_EQUAL(featureId, ft.GetID(), ());
  ExtractRoadInfo(featureId, ft, speedKMPH, ri);
  return ri;
}

FeaturesRoadGraph::Value const & FeaturesRoadGraph::LockMwm(MwmSet::MwmId const & mwmId) const
{
  ASSERT(mwmId.IsAlive(), ());

  auto const itr = m_mwmLocks.find(mwmId);
  if (itr != m_mwmLocks.end())
    return itr->second;

  return m_mwmLocks.insert(make_pair(move(mwmId), Value(m_index, m_index.GetMwmHandleById(mwmId))))
      .first->second;
}
}  // namespace routing
