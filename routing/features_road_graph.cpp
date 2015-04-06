#include "features_road_graph.hpp"
#include "route.hpp"
#include "vehicle_model.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator.hpp"
#include "../indexer/ftypes_matcher.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../base/logging.hpp"

namespace routing
{

uint32_t const FEATURE_CACHE_SIZE = 10;
uint32_t const READ_ROAD_SCALE = 13;
double const READ_CROSS_EPSILON = 1.0E-4;
double const DEFAULT_SPEED_MS = 15.0;

uint32_t indexFound = 0;
uint32_t indexCheck = 0;
uint32_t crossFound = 0;
uint32_t crossCheck = 0;


FeaturesRoadGraph::FeaturesRoadGraph(Index const * pIndex, size_t mwmID)
  : m_pIndex(pIndex), m_mwmID(mwmID), m_vehicleModel(new CarModel()), m_cache(FEATURE_CACHE_SIZE),
    m_cacheMiss(0), m_cacheAccess(0)
{
}

uint32_t FeaturesRoadGraph::GetStreetReadScale()
{
  return READ_ROAD_SCALE;
}

class CrossFeaturesLoader
{
  uint32_t m_featureID;
  FeaturesRoadGraph & m_graph;
  m2::PointD m_point;
  IRoadGraph::TurnsVectorT & m_turns;
  size_t m_count;

public:
  CrossFeaturesLoader(uint32_t fID, FeaturesRoadGraph & graph,
                      m2::PointD const & pt, IRoadGraph::TurnsVectorT & turns)
    : m_featureID(fID), m_graph(graph), m_point(pt), m_turns(turns), m_count(0)
  {
  }

  size_t GetCount() const
  {
    return m_count;
  }

  void operator()(FeatureType & ft)
  {
    FeatureID const fID = ft.GetID();
    if (m_featureID == fID.m_offset || fID.m_mwm != m_graph.GetMwmID())
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

      if (!fc.m_isOneway && (i < count - 1))
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

void FeaturesRoadGraph::GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns, bool noOptimize /*= true*/)
{
  uint32_t const ftID = pos.GetFeatureId();
  FeatureType ft;
  CachedFeature const fc = GetCachedFeature(ftID, ft, true);

  if (fc.m_speed <= 0.0)
    return;

  int const count = static_cast<int>(fc.m_points.size());
  bool const isForward = pos.IsForward();
  int const inc = isForward ? -1 : 1;

  int startID = pos.GetPointId();
  ASSERT_GREATER(count, 1, ());
  ASSERT_LESS(startID, count, ());
  if (!isForward)
    ++startID;

  PossibleTurn thisTurn;
  thisTurn.m_speed = fc.m_speed;
  thisTurn.m_startPoint = fc.m_points[0];
  thisTurn.m_endPoint = fc.m_points[count - 1];

  double distance = 0.0;
  double time = 0.0;
  for (int i = startID; i >= 0 && i < count; i += inc)
  {
    ASSERT_GREATER(i - inc, -1, ());
    ASSERT_LESS(i - inc, count, ());

    double const segmentDistance = CalcDistanceMeters(fc.m_points[i], fc.m_points[i - inc]);
    distance += segmentDistance;
    time += segmentDistance / fc.m_speed;

    m2::PointD const & pt = fc.m_points[i];

    // Find possible turns to point[i] from other features.
    size_t const last = turns.size();
    CrossFeaturesLoader crossLoader(ftID, *this, pt, turns);
    m_pIndex->ForEachInRect(crossLoader,
                            m2::RectD(pt.x - READ_CROSS_EPSILON, pt.y - READ_CROSS_EPSILON,
                                      pt.x + READ_CROSS_EPSILON, pt.y + READ_CROSS_EPSILON),
                            READ_ROAD_SCALE);

    indexCheck++;

    if (crossLoader.GetCount() > 0)
      indexFound++;

    // Skip if there are no turns on point
    if (/*crossLoader.GetCount() > 0 ||*/ noOptimize)
    {
      // Push turn points for this feature.
      if (isForward)
      {
        if (i > 0)
        {
          thisTurn.m_pos = RoadPos(ftID, true, i - 1, pt);
          turns.push_back(thisTurn);
        }
      }
      else
      {
        if (!fc.m_isOneway && (i != count - 1))
        {
          thisTurn.m_pos = RoadPos(ftID, false, i, pt);
          turns.push_back(thisTurn);
        }
      }
    }

    // Update distance and time information.
    for (size_t j = last; j < turns.size(); ++j)
    {
      turns[j].m_metersCovered = distance;
      turns[j].m_secondsCovered = time;
      turns[j].m_speed = DEFAULT_SPEED_MS;
    }
  }

  // Check cycle
  if (m2::AlmostEqual(fc.m_points[0], fc.m_points[count - 1]))
  {
    /// @todo calculate distance and speed
    if (isForward)
    {
      double distance = 0;
      for (int i = pos.GetPointId(); i > 0; --i)
        distance += CalcDistanceMeters(fc.m_points[i], fc.m_points[i - 1]);

      thisTurn.m_pos = RoadPos(ftID, true, count - 2, fc.m_points[count - 1]);
      thisTurn.m_metersCovered = distance;
      thisTurn.m_secondsCovered = distance / DEFAULT_SPEED_MS;
      turns.push_back(thisTurn);
    }
    else if (!fc.m_isOneway)
    {
      double distance = 0;
      for (size_t i = pos.GetPointId(); i < count - 1; ++i)
        distance += CalcDistanceMeters(fc.m_points[i], fc.m_points[i + 1]);

      thisTurn.m_pos = RoadPos(ftID, false, 0, fc.m_points[0]);
      thisTurn.m_metersCovered = distance;
      thisTurn.m_secondsCovered = distance / DEFAULT_SPEED_MS;
      turns.push_back(thisTurn);
    }
  }
}

void FeaturesRoadGraph::ReconstructPath(RoadPosVectorT const & positions, Route & route)
{
  LOG(LINFO, ("Cache miss: ", GetCacheMiss() * 100, " Access count: ", m_cacheAccess));
  LOG(LINFO, ("Index check: ", indexCheck, " Index found: ", indexFound, " (", 100.0 * (double)indexFound / (double)indexCheck, "%)"));
  LOG(LINFO, ("Cross check: ", crossCheck, " Cross found: ", crossFound, " (", 100.0 * (double)crossFound / (double)crossCheck, "%)"));

  size_t count = positions.size();
  if (count < 2)
    return;

  FeatureType ft1;
  vector<m2::PointD> poly;

  // Initialize starting point.
  LoadFeature(positions.back().GetFeatureId(), ft1);

  for (size_t i = count-1; i > 0; --i)
  {
    RoadPos const & pos1 = positions[i];
    RoadPos const & pos2 = positions[i-1];

    FeatureType ft2;

    // Find next connection point
    m2::PointD lastPt;
    bool const diffIDs = pos1.GetFeatureId() != pos2.GetFeatureId();
    if (diffIDs)
    {
      LoadFeature(pos2.GetFeatureId(), ft2);
      lastPt = ft2.GetPoint(pos2.GetPointId() + (pos2.IsForward() ? 1 : 0));
    }
    else
      lastPt = ft1.GetPoint(pos2.GetPointId() + (pos1.IsForward() ? 1 : 0));

    // Accumulate points from start point id to pt.
    int const inc = pos1.IsForward() ? -1 : 1;
    int ptID = pos1.GetPointId() + (pos1.IsForward() ? 0 : 1);
    m2::PointD pt;
    do
    {
      pt = ft1.GetPoint(ptID);
      poly.push_back(pt);

      LOG(LDEBUG, (pt, pos1.GetFeatureId(), ptID));

      ptID += inc;

    } while (!m2::AlmostEqual(pt, lastPt));

    // Assign current processing feature.
    if (diffIDs)
      ft1.SwapGeometry(ft2);
  }

  route.SetGeometry(poly.rbegin(), poly.rend());
}

bool FeaturesRoadGraph::IsOneWay(FeatureType const & ft) const
{
  return m_vehicleModel->IsOneWay(ft);
}

double FeaturesRoadGraph::GetSpeed(FeatureType const & ft) const
{
  return m_vehicleModel->GetSpeed(ft);
}

FeaturesRoadGraph::CachedFeature const & FeaturesRoadGraph::GetCachedFeature(
    uint32_t const ftId, FeatureType & ft, bool fullLoad)
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

} // namespace routing
