#include "routing/features_road_graph.hpp"
#include "routing/route.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"

namespace routing
{

namespace
{
uint32_t const FEATURE_CACHE_SIZE = 10;
double const READ_CROSS_EPSILON = 1.0E-4;
}  // namespace

FeaturesRoadGraph::FeaturesRoadGraph(IVehicleModel const & vehicleModel, Index const & index, MwmSet::MwmId const & mwmID)
    : m_index(index),
      m_mwmID(mwmID),
      m_vehicleModel(vehicleModel),
      m_cache(FEATURE_CACHE_SIZE),
      m_cacheMiss(0),
      m_cacheAccess(0)
{
}

uint32_t FeaturesRoadGraph::GetStreetReadScale() { return scales::GetUpperScale(); }

class CrossFeaturesLoader
{
public:
  CrossFeaturesLoader(FeaturesRoadGraph const & graph,
                      IRoadGraph::CrossEdgesLoader & edgesLoader)
      : m_graph(graph), m_edgesLoader(edgesLoader)
  {}

  void operator()(FeatureType & ft)
  {
    FeatureID const fID = ft.GetID();
    if (fID.m_mwmId != m_graph.GetMwmID())
      return;

    /// @todo remove overhead with type and speed checks (now speed loads in cache creation)
    // check type to skip not line objects
    feature::TypesHolder types(ft);
    if (types.GetGeoType() != feature::GEOM_LINE)
      return;

    // skip roads with null speed
    double const speed = m_graph.GetSpeedKMPHFromFt(ft);
    if (speed <= 0.0)
      return;

    // load feature from cache
    IRoadGraph::RoadInfo const & ri = m_graph.GetCachedRoadInfo(fID.m_offset, ft, false /*fullLoad*/);
    ASSERT_EQUAL(speed, ri.m_speedKMPH, ());

    m_edgesLoader(fID.m_offset, ri);
  }

private:
  FeaturesRoadGraph const & m_graph;
  IRoadGraph::CrossEdgesLoader & m_edgesLoader;
};

void FeaturesRoadGraph::LoadFeature(uint32_t featureId, FeatureType & ft) const
{
  Index::FeaturesLoaderGuard loader(m_index, m_mwmID);
  loader.GetFeature(featureId, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, (featureId));
  ASSERT_GREATER(ft.GetPointsCount(), 1, (featureId));
}

IRoadGraph::RoadInfo FeaturesRoadGraph::GetRoadInfo(uint32_t featureId) const
{
  FeatureType ft;
  return GetCachedRoadInfo(featureId, ft, true /*fullLoad*/);
}

double FeaturesRoadGraph::GetSpeedKMPH(uint32_t featureId) const
{
  FeatureType ft;
  return GetCachedRoadInfo(featureId, ft, true /*fullLoad*/).m_speedKMPH;
}

double FeaturesRoadGraph::GetMaxSpeedKMPH() const
{
  return m_vehicleModel.GetMaxSpeed();
}

void FeaturesRoadGraph::ForEachFeatureClosestToCross(m2::PointD const & cross,
                                                     CrossEdgesLoader & edgesLoader) const
{
  CrossFeaturesLoader featuresLoader(*this, edgesLoader);
  m_index.ForEachInRect(featuresLoader,
                        m2::RectD(cross.x - READ_CROSS_EPSILON, cross.y - READ_CROSS_EPSILON,
                                  cross.x + READ_CROSS_EPSILON, cross.y + READ_CROSS_EPSILON),
                        scales::GetUpperScale());
}

bool FeaturesRoadGraph::IsOneWay(FeatureType const & ft) const
{
  return m_vehicleModel.IsOneWay(ft);
}

double FeaturesRoadGraph::GetSpeedKMPHFromFt(FeatureType const & ft) const
{
  return m_vehicleModel.GetSpeed(ft);
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(uint32_t featureId,
                                                                  FeatureType & ft, bool fullLoad) const
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(featureId, found);

  if (!found)
  {
    if (fullLoad)
      LoadFeature(featureId, ft);
    else
    {
      // ft must be set
      ASSERT(featureId == ft.GetID().m_offset, ());
      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    }

    ri.m_bidirectional = !IsOneWay(ft);
    ri.m_speedKMPH = GetSpeedKMPHFromFt(ft);
    ft.SwapPoints(ri.m_points);
    m_cacheMiss++;
  }
  m_cacheAccess++;

  return ri;
}

}  // namespace routing
