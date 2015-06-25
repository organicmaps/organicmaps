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
uint32_t constexpr kPowOfTwoForFeatureCacheSize = 10; // cache contains 2 ^ kPowOfTwoForFeatureCacheSize elements
double constexpr kReadCrossEpsilon = 1.0E-4;
}  // namespace

FeaturesRoadGraph::FeaturesRoadGraph(IVehicleModel const & vehicleModel, Index const & index, MwmSet::MwmId const & mwmID)
    : m_index(index),
      m_mwmID(mwmID),
      m_vehicleModel(vehicleModel),
      m_cache(kPowOfTwoForFeatureCacheSize)
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
    // check type to skip not line objects
    if (ft.GetFeatureType() != feature::GEOM_LINE)
      return;

    // skip roads with null speed
    double const speedKMPH = m_graph.GetSpeedKMPHFromFt(ft);
    if (speedKMPH <= 0.0)
      return;

    FeatureID const fID = ft.GetID();
    if (fID.m_mwmId != m_graph.GetMwmID())
      return;

    // load feature from cache
    IRoadGraph::RoadInfo const & ri = m_graph.GetCachedRoadInfo(fID.m_offset, ft, speedKMPH);

    m_edgesLoader(fID.m_offset, ri);
  }

private:
  FeaturesRoadGraph const & m_graph;
  IRoadGraph::CrossEdgesLoader & m_edgesLoader;
};

IRoadGraph::RoadInfo FeaturesRoadGraph::GetRoadInfo(uint32_t featureId) const
{
  RoadInfo const & ri = GetCachedRoadInfo(featureId);
  ASSERT_GREATER(ri.m_speedKMPH, 0.0, ());
  return ri;
}

double FeaturesRoadGraph::GetSpeedKMPH(uint32_t featureId) const
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
                                                     CrossEdgesLoader & edgesLoader) const
{
  CrossFeaturesLoader featuresLoader(*this, edgesLoader);
  m_index.ForEachInRect(featuresLoader,
                        m2::RectD(cross.x - kReadCrossEpsilon, cross.y - kReadCrossEpsilon,
                                  cross.x + kReadCrossEpsilon, cross.y + kReadCrossEpsilon),
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

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(uint32_t featureId) const
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(featureId, found);

  if (found)
    return ri;

  FeatureType ft;
  Index::FeaturesLoaderGuard loader(m_index, m_mwmID);
  loader.GetFeature(featureId, ft);
  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, (featureId));

  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ri.m_bidirectional = !IsOneWay(ft);
  ri.m_speedKMPH = GetSpeedKMPHFromFt(ft);
  ft.SwapPoints(ri.m_points);

  return ri;
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(uint32_t featureId,
                                                                  FeatureType & ft,
                                                                  double speedKMPH) const
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(featureId, found);

  if (found)
    return ri;

  // ft must be set
  ASSERT_EQUAL(featureId, ft.GetID().m_offset, ());

  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ri.m_bidirectional = !IsOneWay(ft);
  ri.m_speedKMPH = speedKMPH;
  ft.SwapPoints(ri.m_points);

  return ri;
}

}  // namespace routing
