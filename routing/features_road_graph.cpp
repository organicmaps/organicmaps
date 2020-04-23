#include "routing/features_road_graph.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/route.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include <limits>

using namespace std;

namespace routing
{
namespace
{
uint32_t constexpr kPowOfTwoForFeatureCacheSize = 10; // cache contains 2 ^ kPowOfTwoForFeatureCacheSize elements

double constexpr kMwmRoadCrossingRadiusMeters = 2.0;

auto constexpr kInvalidSpeedKMPH = numeric_limits<double>::max();
}  // namespace

double GetRoadCrossingRadiusMeters() { return kMwmRoadCrossingRadiusMeters; }

FeaturesRoadGraph::Value::Value(DataSource const & dataSource, MwmSet::MwmHandle handle)
  : m_mwmHandle(move(handle))
{
  if (!m_mwmHandle.IsAlive())
    return;

  m_altitudeLoader = make_unique<feature::AltitudeLoader>(dataSource, m_mwmHandle.GetId());
}

FeaturesRoadGraph::CrossCountryVehicleModel::CrossCountryVehicleModel(
    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory)
  : m_vehicleModelFactory(vehicleModelFactory)
  , m_maxSpeed(m_vehicleModelFactory->GetVehicleModel()->GetMaxWeightSpeed())
  , m_offroadSpeedKMpH(m_vehicleModelFactory->GetVehicleModel()->GetOffroadSpeed())
{
}

SpeedKMpH FeaturesRoadGraph::CrossCountryVehicleModel::GetSpeed(
    FeatureType & f, SpeedParams const & speedParams) const
{
  return GetVehicleModel(f.GetID())->GetSpeed(f, speedParams);
}

HighwayType FeaturesRoadGraph::CrossCountryVehicleModel::GetHighwayType(FeatureType & f) const
{
  return GetVehicleModel(f.GetID())->GetHighwayType(f);
}

SpeedKMpH const & FeaturesRoadGraph::CrossCountryVehicleModel::GetOffroadSpeed() const
{
  return m_offroadSpeedKMpH;
}

bool FeaturesRoadGraph::CrossCountryVehicleModel::IsOneWay(FeatureType & f) const
{
  return GetVehicleModel(f.GetID())->IsOneWay(f);
}

bool FeaturesRoadGraph::CrossCountryVehicleModel::IsRoad(FeatureType & f) const
{
  return GetVehicleModel(f.GetID())->IsRoad(f);
}

bool FeaturesRoadGraph::CrossCountryVehicleModel::IsPassThroughAllowed(FeatureType & f) const
{
  return GetVehicleModel(f.GetID())->IsPassThroughAllowed(f);
}

VehicleModelInterface * FeaturesRoadGraph::CrossCountryVehicleModel::GetVehicleModel(
    FeatureID const & featureId) const
{
  auto itr = m_cache.find(featureId.m_mwmId);
  if (itr != m_cache.end())
    return itr->second.get();

  auto const vehicleModel = m_vehicleModelFactory->GetVehicleModelForCountry(
      featureId.m_mwmId.GetInfo()->GetCountryName());

  ASSERT(vehicleModel, ());
  ASSERT_EQUAL(m_maxSpeed, vehicleModel->GetMaxWeightSpeed(), ());

  itr = m_cache.emplace(featureId.m_mwmId, move(vehicleModel)).first;
  return itr->second.get();
}

void FeaturesRoadGraph::CrossCountryVehicleModel::Clear()
{
  m_cache.clear();
}

IRoadGraph::RoadInfo & FeaturesRoadGraph::RoadInfoCache::Find(FeatureID const & featureId, bool & found)
{
  std::lock_guard lock(m_mutexCache);
  auto res = m_cache.emplace(featureId.m_mwmId, TMwmFeatureCache());
  if (res.second)
    res.first->second.Init(kPowOfTwoForFeatureCacheSize);
  return res.first->second.Find(featureId.m_index, found);
}

void FeaturesRoadGraph::RoadInfoCache::Clear()
{
  std::lock_guard lock(m_mutexCache);
  m_cache.clear();
}

FeaturesRoadGraph::FeaturesRoadGraph(DataSource const & dataSource, IRoadGraph::Mode mode,
                                     shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory)
  : m_dataSource(dataSource), m_mode(mode), m_vehicleModel(vehicleModelFactory)
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

    FeatureID const & featureId = ft.GetID();

    IRoadGraph::RoadInfo const & roadInfo =
        m_graph.GetCachedRoadInfo(featureId, ft, kInvalidSpeedKMPH);
    CHECK_EQUAL(roadInfo.m_speedKMPH, kInvalidSpeedKMPH, ());

    m_edgesLoader(featureId, roadInfo.m_junctions, roadInfo.m_bidirectional);
  }

private:
  FeaturesRoadGraph const & m_graph;
  IRoadGraph::ICrossEdgesLoader & m_edgesLoader;
};

IRoadGraph::RoadInfo FeaturesRoadGraph::GetRoadInfo(FeatureID const & featureId,
                                                    SpeedParams const & speedParams) const
{
  RoadInfo const & ri = GetCachedRoadInfo(featureId, speedParams);
  ASSERT_GREATER(ri.m_speedKMPH, 0.0, ());
  return ri;
}

double FeaturesRoadGraph::GetSpeedKMpH(FeatureID const & featureId, SpeedParams const & speedParams) const
{
  double const speedKMPH = GetCachedRoadInfo(featureId, speedParams).m_speedKMPH;
  ASSERT_GREATER(speedKMPH, 0.0, ());
  return speedKMPH;
}

double FeaturesRoadGraph::GetMaxSpeedKMpH() const { return m_vehicleModel.GetMaxWeightSpeed(); }

void FeaturesRoadGraph::ForEachFeatureClosestToCross(m2::PointD const & cross,
                                                     ICrossEdgesLoader & edgesLoader) const
{
  CrossFeaturesLoader featuresLoader(*this, edgesLoader);
  m2::RectD const rect =
      mercator::RectByCenterXYAndSizeInMeters(cross, kMwmRoadCrossingRadiusMeters);
  m_dataSource.ForEachInRect(featuresLoader, rect, GetStreetReadScale());
}

void FeaturesRoadGraph::FindClosestEdges(
    m2::RectD const & rect, uint32_t count,
    vector<pair<Edge, geometry::PointWithAltitude>> & vicinities) const
{
  NearestEdgeFinder finder(rect.Center(), nullptr /* IsEdgeProjGood */);

  auto const f = [&finder, this](FeatureType & ft)
  {
    if (!m_vehicleModel.IsRoad(ft))
      return;

    FeatureID const & featureId = ft.GetID();

    IRoadGraph::RoadInfo const & roadInfo = GetCachedRoadInfo(featureId, ft, kInvalidSpeedKMPH);
    finder.AddInformationSource(IRoadGraph::FullRoadInfo(featureId, roadInfo));
  };

  m_dataSource.ForEachInRect(f, rect, GetStreetReadScale());

  finder.MakeResult(vicinities, count);
}

vector<IRoadGraph::FullRoadInfo>
FeaturesRoadGraph::FindRoads(m2::RectD const & rect, IsGoodFeatureFn const & isGoodFeature) const
{
  vector<IRoadGraph::FullRoadInfo> roads;
  auto const f = [&roads, &isGoodFeature, &rect, this](FeatureType & ft) {
    if (!m_vehicleModel.IsRoad(ft))
      return;

    FeatureID const & featureId = ft.GetID();
    if (isGoodFeature && !isGoodFeature(featureId))
      return;

    // DataSource::ForEachInRect() gives not only features inside |rect| but some other features
    // which lie close to the rect. Removes all the features which don't cross |rect|.
    auto const & roadInfo = GetCachedRoadInfo(featureId, ft, kInvalidSpeedKMPH);
    if (!RectCoversPolyline(roadInfo.m_junctions, rect))
      return;

    roads.emplace_back(featureId, roadInfo);
  };

  m_dataSource.ForEachInRect(f, rect, GetStreetReadScale());
  return roads;
}

void FeaturesRoadGraph::GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const
{
  FeaturesLoaderGuard loader(m_dataSource, featureId.m_mwmId);
  auto ft = loader.GetFeatureByIndex(featureId.m_index);
  if (!ft)
    return;

  ASSERT_EQUAL(ft->GetGeomType(), feature::GeomType::Line, ());
  types = feature::TypesHolder(*ft);
}

void FeaturesRoadGraph::GetJunctionTypes(geometry::PointWithAltitude const & junction,
                                         feature::TypesHolder & types) const
{
  types = feature::TypesHolder();

  m2::PointD const & cross = junction.GetPoint();

  auto const f = [&types, &cross](FeatureType & ft) {
    if (!types.Empty())
      return;

    if (ft.GetGeomType() != feature::GeomType::Point)
      return;

    if (!base::AlmostEqualAbs(ft.GetCenter(), cross, kMwmPointAccuracy))
      return;

    feature::TypesHolder typesHolder(ft);
    if (!typesHolder.Empty())
      types = typesHolder;
  };

  m2::RectD const rect =
      mercator::RectByCenterXYAndSizeInMeters(cross, kMwmRoadCrossingRadiusMeters);
  m_dataSource.ForEachInRect(f, rect, GetStreetReadScale());
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

bool FeaturesRoadGraph::IsRoad(FeatureType & ft) const { return m_vehicleModel.IsRoad(ft); }

IRoadGraph::PointWithAltitudeVec FeaturesRoadGraph::GetRoadGeom(FeatureType & ft) const
{
  FeatureID const & featureId = ft.GetID();
  IRoadGraph::RoadInfo const & roadInfo = GetCachedRoadInfo(featureId, ft, kInvalidSpeedKMPH);
  CHECK_EQUAL(roadInfo.m_speedKMPH, kInvalidSpeedKMPH, ());
  return roadInfo.m_junctions;
}

bool FeaturesRoadGraph::IsOneWay(FeatureType & ft) const { return m_vehicleModel.IsOneWay(ft); }

double FeaturesRoadGraph::GetSpeedKMpHFromFt(FeatureType & ft, SpeedParams const & speedParams) const
{
  return m_vehicleModel.GetSpeed(ft, speedParams).m_weight;
}

void FeaturesRoadGraph::ExtractRoadInfo(FeatureID const & featureId, FeatureType & ft,
                                        double speedKMpH, RoadInfo & ri) const
{
  ri.m_speedKMPH = speedKMpH;

  Value const & value = LockMwm(featureId.m_mwmId);
  if (!value.IsAlive())
    return;

  ri.m_bidirectional = !IsOneWay(ft);

  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
  size_t const pointsCount = ft.GetPointsCount();

  geometry::Altitudes altitudes;
  if (value.m_altitudeLoader)
  {
    altitudes = value.m_altitudeLoader->GetAltitudes(featureId.m_index, ft.GetPointsCount());
  }
  else
  {
    ASSERT(false, ());
    altitudes = geometry::Altitudes(ft.GetPointsCount(), geometry::kDefaultAltitudeMeters);
  }

  CHECK_EQUAL(altitudes.size(), pointsCount,
              ("altitudeLoader->GetAltitudes(", featureId.m_index, "...) returns wrong altitudes:",
               altitudes));

  ri.m_junctions.resize(pointsCount);
  for (size_t i = 0; i < pointsCount; ++i)
    ri.m_junctions[i] = geometry::PointWithAltitude(ft.GetPoint(i), altitudes[i]);
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(FeatureID const & featureId,
                                                                  SpeedParams const & speedParams) const
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(featureId, found);

  if (found)
    return ri;

  FeaturesLoaderGuard loader(m_dataSource, featureId.m_mwmId);

  auto ft = loader.GetFeatureByIndex(featureId.m_index);
  if (!ft)
    return ri;

  ASSERT_EQUAL(ft->GetGeomType(), feature::GeomType::Line, ());

  ExtractRoadInfo(featureId, *ft, GetSpeedKMpHFromFt(*ft, speedParams), ri);
  return ri;
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(FeatureID const & featureId,
                                                                  FeatureType & ft,
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

  return m_mwmLocks.emplace(move(mwmId), Value(m_dataSource, m_dataSource.GetMwmHandleById(mwmId)))
      .first->second;
}
}  // namespace routing
