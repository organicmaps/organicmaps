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

/// @todo Factor out vehicle model as a parameter for the features graph.
FeaturesRoadGraph::FeaturesRoadGraph(Index const * pIndex, size_t mwmID)
    : m_pIndex(pIndex),
      m_mwmID(mwmID),
      m_vehicleModel(new PedestrianModel()),
      m_cache(FEATURE_CACHE_SIZE),
      m_cacheMiss(0),
      m_cacheAccess(0)
{
}

uint32_t FeaturesRoadGraph::GetStreetReadScale() { return scales::GetUpperScale(); }

class CrossFeaturesLoader
{
public:
  CrossFeaturesLoader(FeaturesRoadGraph & graph, m2::PointD const & point,
                      IRoadGraph::CrossTurnsLoader & turnsLoader)
      : m_graph(graph), m_point(point), m_turnsLoader(turnsLoader)
  {
  }

  void operator()(FeatureType & ft)
  {
    FeatureID const fID = ft.GetID();
    if (fID.m_mwm != m_graph.GetMwmID())
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
    IRoadGraph::RoadInfo const & ri = m_graph.GetCachedRoadInfo(fID.m_offset, ft, false);
    ASSERT_EQUAL(speed, ri.m_speedKMPH, ());

    m_turnsLoader(fID.m_offset, ri);
  }

private:
  FeaturesRoadGraph & m_graph;
  m2::PointD m_point;
  IRoadGraph::CrossTurnsLoader & m_turnsLoader;
};

void FeaturesRoadGraph::LoadFeature(uint32_t featureId, FeatureType & ft)
{
  Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmID);
  loader.GetFeature(featureId, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, (featureId));
  ASSERT_GREATER(ft.GetPointsCount(), 1, (featureId));
}

IRoadGraph::RoadInfo FeaturesRoadGraph::GetRoadInfo(uint32_t featureId)
{
  FeatureType ft;
  return GetCachedRoadInfo(featureId, ft, true);
}

double FeaturesRoadGraph::GetSpeedKMPH(uint32_t featureId)
{
  FeatureType ft;
  LoadFeature(featureId, ft);
  return GetSpeedKMPHFromFt(ft);
}

void FeaturesRoadGraph::ForEachClosestToCrossFeature(m2::PointD const & cross,
                                                     CrossTurnsLoader & turnsLoader)
{
  CrossFeaturesLoader featuresLoader(*this, cross, turnsLoader);
  m_pIndex->ForEachInRect(featuresLoader,
                          m2::RectD(cross.x - READ_CROSS_EPSILON, cross.y - READ_CROSS_EPSILON,
                                    cross.x + READ_CROSS_EPSILON, cross.y + READ_CROSS_EPSILON),
                          scales::GetUpperScale());
}

bool FeaturesRoadGraph::IsOneWay(FeatureType const & ft) const
{
  return m_vehicleModel->IsOneWay(ft);
}

double FeaturesRoadGraph::GetSpeedKMPHFromFt(FeatureType const & ft) const
{
  return m_vehicleModel->GetSpeed(ft);
}

IRoadGraph::RoadInfo const & FeaturesRoadGraph::GetCachedRoadInfo(uint32_t const ftId,
                                                                  FeatureType & ft, bool fullLoad)
{
  bool found = false;
  RoadInfo & ri = m_cache.Find(ftId, found);

  if (!found)
  {
    if (fullLoad)
      LoadFeature(ftId, ft);
    else
      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    ri.m_bidirectional = !IsOneWay(ft);
    ri.m_speedKMPH = GetSpeedKMPHFromFt(ft);
    ft.SwapPoints(ri.m_points);
    m_cacheMiss++;
  }
  m_cacheAccess++;

  return ri;
}
}  // namespace routing
