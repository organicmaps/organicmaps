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
namespace
{
uint32_t const FEATURE_CACHE_SIZE = 10;
uint32_t const READ_ROAD_SCALE = 13;
double const READ_CROSS_EPSILON = 1.0E-4;
double const KMPH2MPS = 1000.0 / (60 * 60);

uint32_t indexFound = 0;
uint32_t indexCheck = 0;
uint32_t crossFound = 0;
uint32_t crossCheck = 0;
}  // namespace

/// @todo Factor out vehicle model as a parameter for the features graph.
FeaturesRoadGraph::FeaturesRoadGraph(Index const * pIndex, size_t mwmID)
  : m_pIndex(pIndex), m_mwmID(mwmID), m_vehicleModel(new PedestrianModel()), m_cache(FEATURE_CACHE_SIZE),
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

  size_t const numPoints = fc.m_points.size();
  ASSERT_GREATER_OR_EQUAL(numPoints, 2, ("Incorrect road - only", numPoints, "point(s)."));

  bool const isForward = pos.IsForward();
  uint32_t const segId = pos.GetSegId();

  double const segmentDistance =
      CalcDistanceMeters(pos.GetSegEndpoint(), fc.m_points[pos.GetSegStartPointId()]);
  double const segmentTime = segmentDistance / fc.m_speed;

  m2::PointD const point = fc.m_points[pos.GetSegStartPointId()];

  // Find possible turns to startPoint from other features.
  CrossFeaturesLoader crossLoader(featureId, *this, point, turns);
  m_pIndex->ForEachInRect(crossLoader,
                          m2::RectD(point.x - READ_CROSS_EPSILON, point.y - READ_CROSS_EPSILON,
                                    point.x + READ_CROSS_EPSILON, point.y + READ_CROSS_EPSILON),
                          READ_ROAD_SCALE);

  indexCheck++;

  if (crossLoader.GetCount() > 0)
    indexFound++;

  {
    PossibleTurn revTurn;
    revTurn.m_speed = fc.m_speed;
    revTurn.m_metersCovered = 0;
    revTurn.m_secondsCovered = 0;
    revTurn.m_startPoint = fc.m_points.front();
    revTurn.m_endPoint = fc.m_points.back();
    revTurn.m_pos =
        RoadPos(pos.GetFeatureId(), !pos.IsForward(), pos.GetSegId(), pos.GetSegEndpoint());
    turns.push_back(revTurn);
  }

  // Following code adds "fake" turn from an adjacent segment to pos.
  // That's why m_startPoint and m_endPoint are set to the bounds of
  // the current road.
  PossibleTurn thisTurn;
  thisTurn.m_speed = fc.m_speed;
  thisTurn.m_metersCovered = segmentDistance;
  thisTurn.m_secondsCovered = segmentTime;
  thisTurn.m_startPoint = fc.m_points.front();
  thisTurn.m_endPoint = fc.m_points.back();

  // Push turn points for this feature.
  if (isForward && segId > 0)
  {
    thisTurn.m_pos = RoadPos(featureId, isForward, segId - 1, point);
    turns.push_back(thisTurn);
  }
  if (!isForward && segId + 2 < numPoints)
  {
    thisTurn.m_pos = RoadPos(featureId, isForward, segId + 1, point);
    turns.push_back(thisTurn);
  }

  // Checks that the road is not a loop.
  if (!m2::AlmostEqual(fc.m_points.front(), fc.m_points.back()))
    return;

  if (isForward && segId == 0)
  {
    // It seems that it's possible to get from the last segment to the first.
    thisTurn.m_pos = RoadPos(featureId, isForward, numPoints - 2 /* segId */, point);
    turns.push_back(thisTurn);
  }
  if (!isForward && segId + 2 == numPoints)
  {
    // It seems that it's possible to get from the first segment to the last.
    thisTurn.m_pos = RoadPos(featureId, isForward, 0 /* segId */, point);
    turns.push_back(thisTurn);
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

  double trackTime = 0;

  // Initialize starting point.
  LoadFeature(positions.back().GetFeatureId(), ft1);

  m2::PointD prevPoint = positions.back().GetSegEndpoint();
  poly.push_back(prevPoint);

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
      lastPt = ft2.GetPoint(pos2.GetSegEndPointId());
    }
    else
      lastPt = ft1.GetPoint(pos2.GetSegEndPointId());

    // Accumulate points from start point id to pt.
    int const inc = pos1.IsForward() ? -1 : 1;
    int ptID = pos1.GetSegStartPointId();
    m2::PointD curPoint;
    double segmentLengthM = 0.0;
    do
    {
      curPoint = ft1.GetPoint(ptID);
      poly.push_back(curPoint);
      segmentLengthM += CalcDistanceMeters(curPoint, prevPoint);
      prevPoint = curPoint;

      LOG(LDEBUG, (curPoint, pos1.GetFeatureId(), ptID));

      ptID += inc;

    } while (ptID >= 0 && ptID < ft1.GetPointsCount() && !m2::AlmostEqual(curPoint, lastPt));

    segmentLengthM += CalcDistanceMeters(lastPt, prevPoint);
    // Calculation total feature time. Seconds.
    trackTime += segmentLengthM / (m_vehicleModel->GetSpeed(ft1) * KMPH2MPS);

    // Assign current processing feature.
    if (diffIDs)
      ft1.SwapGeometry(ft2);
  }

  poly.push_back(positions.front().GetSegEndpoint());
  trackTime +=
      CalcDistanceMeters(poly.back(), prevPoint) / (m_vehicleModel->GetSpeed(ft1) * KMPH2MPS);

  ASSERT_GREATER(poly.size(), 1, ("Empty track"));

  if (poly.size() > 1)
  {
    Route::TurnsT turnsDir;
    Route::TimesT times;

    times.emplace_back(poly.size() - 1, trackTime);
    turnsDir.emplace_back(poly.size() - 1, turns::ReachedYourDestination);

    route.SetGeometry(poly.rbegin(), poly.rend());
    route.SetTurnInstructions(turnsDir);
    route.SetSectionTimes(times);
  }
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
