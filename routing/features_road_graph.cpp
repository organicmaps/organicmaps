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
  FeaturesRoadGraph & m_graph;
  m2::PointD m_point;
  IRoadGraph::TurnsVectorT & m_turns;
  size_t m_count;

public:
  CrossFeaturesLoader(FeaturesRoadGraph & graph, m2::PointD const & pt,
                      IRoadGraph::TurnsVectorT & turns)
      : m_graph(graph), m_point(pt), m_turns(turns), m_count(0)
  {
  }

  size_t GetCount() const { return m_count; }

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
    FeaturesRoadGraph::CachedFeature const & fc = m_graph.GetCachedFeature(fID.m_offset, ft, false);
    ASSERT_EQUAL(speed, fc.m_speed, ());

    size_t const count = fc.m_points.size();

    PossibleTurn t;
    t.m_speed = fc.m_speed;
    t.m_startPoint = fc.m_points[0];
    t.m_endPoint = fc.m_points[count - 1];

    for (size_t i = 0; i < count; ++i)
    {
      m2::PointD const & p = fc.m_points[i];

      /// @todo Is this a correct way to compare?
      if (!m2::AlmostEqual(m_point, p))
        continue;

      if (i > 0)
      {
        ++m_count;
        t.m_pos = RoadPos(fID.m_offset, true, i - 1, p);
        m_turns.push_back(t);
      }

      if (i < count - 1)
      {
        ++m_count;
        t.m_pos = RoadPos(fID.m_offset, false, i, p);
        m_turns.push_back(t);
      }
    }
  }
};

void FeaturesRoadGraph::LoadFeature(uint32_t featureId, FeatureType & ft)
{
  Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmID);
  loader.GetFeature(featureId, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, (featureId));
  ASSERT_GREATER(ft.GetPointsCount(), 1, (featureId));
}

void FeaturesRoadGraph::GetNearestTurns(RoadPos const & pos, vector<PossibleTurn> & turns)
{
  uint32_t const featureId = pos.GetFeatureId();
  FeatureType ft;
  CachedFeature const fc = GetCachedFeature(featureId, ft, true);

  if (fc.m_speed <= 0.0)
    return;

  ASSERT_GREATER_OR_EQUAL(fc.m_points.size(), 2,
                          ("Incorrect road - only", fc.m_points.size(), "point(s)."));

  m2::PointD const point = fc.m_points[pos.GetSegStartPointId()];

  // Find possible turns to startPoint from other features.
  CrossFeaturesLoader crossLoader(*this, point, turns);
  m_pIndex->ForEachInRect(crossLoader,
                          m2::RectD(point.x - READ_CROSS_EPSILON, point.y - READ_CROSS_EPSILON,
                                    point.x + READ_CROSS_EPSILON, point.y + READ_CROSS_EPSILON),
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

double FeaturesRoadGraph::GetSpeedKMPH(uint32_t featureId)
{
  FeatureType ft;
  LoadFeature(featureId, ft);
  return GetSpeedKMPHFromFt(ft);
}

FeaturesRoadGraph::CachedFeature const & FeaturesRoadGraph::GetCachedFeature(uint32_t const ftId,
                                                                             FeatureType & ft,
                                                                             bool fullLoad)
{
  bool found = false;
  CachedFeature & f = m_cache.Find(ftId, found);

  if (!found)
  {
    if (fullLoad)
      LoadFeature(ftId, ft);
    else
      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    f.m_isOneway = IsOneWay(ft);
    f.m_speed = GetSpeedKMPHFromFt(ft);
    ft.SwapPoints(f.m_points);
    m_cacheMiss++;
  }
  m_cacheAccess++;

  return f;
}
}  // namespace routing
