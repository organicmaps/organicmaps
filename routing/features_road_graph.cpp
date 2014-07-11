#include "features_road_graph.hpp"
#include "route.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator.hpp"
#include "../indexer/feature_data.hpp"
#include "../indexer/ftypes_matcher.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../base/logging.hpp"

namespace routing
{

uint32_t const READ_ROAD_SCALE = 13;
double const READ_CROSS_RADIUS = 10.0;
double const DEFAULT_SPEED_MS = 15.0;


FeatureRoadGraph::FeatureRoadGraph(Index const * pIndex, size_t mwmID)
  : m_pIndex(pIndex), m_mwmID(mwmID)
{
}

uint32_t FeatureRoadGraph::GetStreetReadScale()
{
  return READ_ROAD_SCALE;
}

class CrossFeaturesLoader
{
  uint32_t m_featureID;
  FeatureRoadGraph & m_graph;
  m2::PointD m_point;
  IRoadGraph::TurnsVectorT & m_turns;
  size_t m_count;

public:
  CrossFeaturesLoader(uint32_t fID, FeatureRoadGraph & graph,
                      m2::PointD const & pt, IRoadGraph::TurnsVectorT & turns)
    : m_featureID(fID), m_graph(graph), m_point(pt), m_turns(turns), m_count(0)
  {
  }

  size_t GetCount() const
  {
    return m_count;
  }

  void operator()(FeatureType const & ft)
  {
    FeatureID fID = ft.GetID();
    if (m_featureID == fID.m_offset)
      return;

    feature::TypesHolder types(ft);
    if (!m_graph.IsStreet(types))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    bool const isOneWay = m_graph.IsOneway(types);
    size_t const count = ft.GetPointsCount();

    PossibleTurn t;
    t.m_startPoint = ft.GetPoint(0);
    t.m_endPoint = ft.GetPoint(count - 1);

    for (size_t i = 0; i < count; ++i)
    {
      m2::PointD const & p = ft.GetPoint(i);

      /// @todo Is this a correct way to compare?
      if (!m2::AlmostEqual(m_point, p))
        continue;

      if (i > 0)
      {
        ++m_count;
        t.m_pos = RoadPos(fID.m_offset, true, i - 1);
        m_turns.push_back(t);
      }

      if (!isOneWay && (i < count - 1))
      {
        ++m_count;
        t.m_pos = RoadPos(fID.m_offset, false, i);
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

void FeatureRoadGraph::LoadFeature(uint32_t id, FeatureType & ft)
{
  Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmID);
  loader.GetFeature(id, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
}

void FeatureRoadGraph::GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns)
{
  uint32_t const ftId = pos.GetFeatureId();
  FeatureType ft;
  LoadFeature(ftId, ft);

  int const count = static_cast<int>(ft.GetPointsCount());
  bool const isForward = pos.IsForward();
  bool const isOneWay = IsOneway(ft);
  int const inc = isForward ? -1 : 1;

  int startID = pos.GetPointId();
  ASSERT_LESS(startID, count, ());
  if (!isForward)
    ++startID;

  PossibleTurn thisTurn;
  thisTurn.m_startPoint = ft.GetPoint(0);
  thisTurn.m_endPoint = ft.GetPoint(count - 1);

  double distance = 0.0;
  double time = 0.0;
  for (int i = startID; i >= 0 && i < count; i += inc)
  {
    ASSERT_GREATER(i - inc, -1, ());
    ASSERT_LESS(i - inc, count, ());

    distance += CalcDistanceMeters(ft.GetPoint(i), ft.GetPoint(i - inc));
    time += distance / DEFAULT_SPEED_MS;

    m2::PointD const & pt = ft.GetPoint(i);

    // Find possible turns to point[i] from other features.
    size_t const last = turns.size();
    CrossFeaturesLoader crossLoader(ftId, *this, pt, turns);
    m_pIndex->ForEachInRect(crossLoader,
                            MercatorBounds::RectByCenterXYAndSizeInMeters(pt, READ_CROSS_RADIUS),
                            READ_ROAD_SCALE);

    // Skip if there are no turns on point
    if (crossLoader.GetCount() > 0)
    {
      // Push turn points for this feature.
      if (isForward)
      {
        if (i > 0)
        {
          thisTurn.m_pos = RoadPos(ftId, true, i - 1);
          turns.push_back(thisTurn);
        }
      }
      else
      {
        if (!isOneWay && (i != count - 1))
        {
          thisTurn.m_pos = RoadPos(ftId, false, i);
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
  if (m2::AlmostEqual(ft.GetPoint(0), ft.GetPoint(count - 1)))
  {
    /// @todo calculate distance and speed
    if (isForward)
    {
      double distance = 0;
      for (int i = pos.GetPointId(); i > 0; --i)
        distance += CalcDistanceMeters(ft.GetPoint(i), ft.GetPoint(i - 1));

      thisTurn.m_pos = RoadPos(ftId, true, count - 2);
      thisTurn.m_metersCovered = distance;
      thisTurn.m_secondsCovered = distance / DEFAULT_SPEED_MS;
      turns.push_back(thisTurn);
    }
    else if (!isOneWay)
    {
      double distance = 0;
      for (size_t i = pos.GetPointId(); i < count - 1; ++i)
        distance += CalcDistanceMeters(ft.GetPoint(i), ft.GetPoint(i + 1));

      thisTurn.m_pos = RoadPos(ftId, false, 0);
      thisTurn.m_metersCovered = distance;
      thisTurn.m_secondsCovered = distance / DEFAULT_SPEED_MS;
      turns.push_back(thisTurn);
    }
  }
}

void FeatureRoadGraph::ReconstructPath(RoadPosVectorT const & positions, Route & route)
{
  size_t count = positions.size();
  if (count < 2)
    return;

  FeatureType ft1;
  vector<m2::PointD> poly;

  // Initialize starting point.
  RoadPos pos1 = positions[0];
  LoadFeature(pos1.GetFeatureId(), ft1);

  size_t i = 0;
  while (i < count-1)
  {
    RoadPos const & pos2 = positions[i+1];

    FeatureType ft2;

    // Find next connection point
    m2::PointD lastPt;
    bool const diffIDs = pos1.GetFeatureId() != pos2.GetFeatureId();
    if (diffIDs)
    {
      LoadFeature(pos2.GetFeatureId(), ft2);
      lastPt = ft2.GetPoint(pos2.GetPointId() + (pos2.IsForward() ? 0 : 1));
    }
    else
      lastPt = ft1.GetPoint(pos2.GetPointId() + (pos1.IsForward() ? 0 : 1));

    // Accumulate points from start point id to pt.
    int const inc = pos1.IsForward() ? 1 : -1;
    int ptID = pos1.GetPointId() + (pos1.IsForward() ? 1 : 0);
    m2::PointD pt;
    bool notEnd, notEqualPoints;
    do
    {
      pt = ft1.GetPoint(ptID);
      poly.push_back(pt);

      LOG(LDEBUG, (pt, pos1.GetFeatureId(), ptID));

      ptID += inc;

      notEqualPoints = !m2::AlmostEqual(pt, lastPt);
      notEnd = (ptID >= 0 && ptID < ft1.GetPointsCount());
    } while (notEnd && notEqualPoints);

    // If we reached the end of feature, start with the begining
    // (end - for backward direction) of the next feauture, until reach pos2.
    if (!notEnd && notEqualPoints)
    {
      pos1 = RoadPos(pos2.GetFeatureId(), pos2.IsForward(),
                     pos2.IsForward() ? 0 : ft2.GetPointsCount() - 2);

    }
    else
    {
      pos1 = pos2;
      ++i;
    }

    // Assign current processing feature.
    if (diffIDs)
      ft1.SwapGeometry(ft2);
  }

  route = Route("", poly, "");
}

bool FeatureRoadGraph::IsStreet(feature::TypesHolder const & types) const
{
  return (types.GetGeoType() == feature::GEOM_LINE &&
          ftypes::IsStreetChecker::Instance()(types));
}

bool FeatureRoadGraph::IsOneway(feature::TypesHolder const & types) const
{
  return ftypes::IsStreetChecker::Instance().IsOneway(types);
}

} // namespace routing
