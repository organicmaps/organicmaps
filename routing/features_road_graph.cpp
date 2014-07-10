#include "features_road_graph.hpp"

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


FeatureRoadGraph::FeatureRoadGraph(Index * pIndex, size_t mwmID)
  : m_pIndex(pIndex), m_mwmID(mwmID)
{
  m_onewayType = classif().GetTypeByPath(vector<string>(1, "oneway"));
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

void FeatureRoadGraph::GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns)
{
  Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmID);

  uint32_t const ftId = pos.GetFeatureId();
  FeatureType ft;
  loader.GetFeature(ftId, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

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
    if (i != startID)
    {
      distance += CalcDistanceMeters(ft.GetPoint(i), ft.GetPoint(i - inc));
      time += distance / DEFAULT_SPEED_MS;
    }

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

double FeatureRoadGraph::GetFeatureDistance(RoadPos const & p1, RoadPos const & p2)
{
  /// @todo Implement distance calculation
  return 0.0;
}

void FeatureRoadGraph::ReconstructPath(RoadPosVectorT const & positions, PointsVectorT & poly)
{
  /// @todo Implement path reconstruction
}

bool FeatureRoadGraph::IsStreet(feature::TypesHolder const & types) const
{
  static ftypes::IsStreetChecker const checker;
  return checker(types);
}

bool FeatureRoadGraph::IsOneway(feature::TypesHolder const & types) const
{
  return types.Has(m_onewayType);
}

} // namespace routing
