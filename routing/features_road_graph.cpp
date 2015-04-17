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
double const KMPH2MPS = 1000.0 / (60 * 60);

uint32_t indexFound = 0;
uint32_t indexCheck = 0;
uint32_t crossFound = 0;
uint32_t crossCheck = 0;
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
    double const speed = m_graph.GetSpeed(ft);
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

      crossCheck++;

      /// @todo Is this a correct way to compare?
      if (!m2::AlmostEqual(m_point, p))
        continue;

      crossFound++;

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

double CalcDistanceMeters(m2::PointD const & p1, m2::PointD const & p2)
{
  return ms::DistanceOnEarth(MercatorBounds::YToLat(p1.y), MercatorBounds::XToLon(p1.x),
                             MercatorBounds::YToLat(p2.y), MercatorBounds::XToLon(p2.x));
}

void FeaturesRoadGraph::LoadFeature(uint32_t id, FeatureType & ft)
{
  Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmID);
  loader.GetFeature(id, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, (id));
  ASSERT_GREATER(ft.GetPointsCount(), 1, (id));
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

  indexCheck++;

  if (crossLoader.GetCount() > 0)
    indexFound++;
}

void FeaturesRoadGraph::ReconstructPath(RoadPosVectorT const & positions, Route & route)
{
  LOG(LINFO, ("Cache miss: ", GetCacheMiss() * 100, " Access count: ", m_cacheAccess));
  double const indexCheckRatio =
      indexCheck == 0 ? 0.0 : static_cast<double>(indexFound) / static_cast<double>(indexCheck);
  LOG(LINFO, ("Index check: ", indexCheck, " Index found: ", indexFound, " (",
              100.0 * indexCheckRatio, "%)"));
  double const crossCheckRatio =
      crossCheck == 0 ? 0.0 : static_cast<double>(crossFound) / static_cast<double>(crossCheck);
  LOG(LINFO, ("Cross check: ", crossCheck, " Cross found: ", crossFound, " (",
              100.0 * crossCheckRatio, "%)"));

  if (positions.size() < 2)
  {
    LOG(LERROR, ("Not enough positions to reconstruct a path."));
    return;
  }

  vector<m2::PointD> poly;

  FeatureType prevFt;
  uint32_t prevFtId = positions.front().GetFeatureId();
  LoadFeature(prevFtId, prevFt);

  double trackTime = 0.0;
  m2::PointD prevPoint = positions.front().GetSegEndpoint();
  poly.push_back(prevPoint);
  for (size_t i = 1; i < positions.size(); ++i)
  {
    m2::PointD curPoint = positions[i].GetSegEndpoint();
    poly.push_back(curPoint);

    double const lengthM = CalcDistanceMeters(prevPoint, curPoint);
    trackTime += lengthM / (m_vehicleModel->GetSpeed(prevFt) * KMPH2MPS);

    prevPoint = curPoint;
    if (positions[i].GetFeatureId() != prevFtId)
    {
      LoadFeature(positions[i].GetFeatureId(), prevFt);
      prevFtId = positions[i].GetFeatureId();
    }
  }

  if (poly.size() <= 1)
  {
    ASSERT(false, ("Empty track"));
    return;
  }
  Route::TurnsT turnsDir;
  Route::TimesT times;

  times.emplace_back(poly.size() - 1, trackTime);
  turnsDir.emplace_back(poly.size() - 1, turns::ReachedYourDestination);

  route.SetGeometry(poly.begin(), poly.end());
  route.SetTurnInstructions(turnsDir);
  route.SetSectionTimes(times);
}

bool FeaturesRoadGraph::IsOneWay(FeatureType const & ft) const
{
  return m_vehicleModel->IsOneWay(ft);
}

double FeaturesRoadGraph::GetSpeed(FeatureType const & ft) const
{
  return m_vehicleModel->GetSpeed(ft);
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
    f.m_speed = GetSpeed(ft);
    ft.SwapPoints(f.m_points);
    m_cacheMiss++;
  }
  m_cacheAccess++;

  return f;
}
}  // namespace routing
