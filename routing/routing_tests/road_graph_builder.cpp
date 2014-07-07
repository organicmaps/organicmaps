#include "road_graph_builder.hpp"

#include "../../std/algorithm.hpp"

using namespace routing;

namespace routing_test
{

void RoadInfo::Swap(RoadInfo & r)
{
  m_points.swap(r.m_points);
  std::swap(m_speedMS, r.m_speedMS);
  std::swap(m_bothSides, r.m_bothSides);
}

void RoadGraphMockSource::AddRoad(RoadInfo & rd)
{
  /// @todo Do ASSERT for RoadInfo params.

  uint32_t const id = m_roads.size();

  // Count of sections! (not points)
  size_t count = rd.m_points.size();
  ASSERT_GREATER(count, 1, ());

  for (size_t i = 0; i < count; ++i)
  {
    TurnsMapT::iterator j = m_turns.insert(make_pair(rd.m_points[i], TurnsVectorT())).first;

    PossibleTurn t;
    t.m_startPoint = rd.m_points.front();
    t.m_endPoint = rd.m_points.back();
    t.m_speed = rd.m_speedMS;

    if (i > 0)
    {
      t.m_pos = RoadPos(id, true, i - 1);
      j->second.push_back(t);
    }

    if (rd.m_bothSides && (i < count - 1))
    {
      t.m_pos = RoadPos(id, false, i);
      j->second.push_back(t);
    }
  }

  m_roads.push_back(RoadInfo());
  m_roads.back().Swap(rd);
}

void RoadGraphMockSource::GetPossibleTurns(RoadPos const & pos, TurnsVectorT & turns)
{
  uint32_t const fID = pos.GetFeatureId();
  ASSERT_LESS(fID, m_roads.size(), ());

  vector<m2::PointD> const & points = m_roads[fID].m_points;

  int const inc = pos.IsForward() ? -1 : 1;
  int startID = pos.GetPointId();
  int const count = static_cast<int>(points.size());

  if (!pos.IsForward())
    ++startID;

  double const speed = m_roads[fID].m_speedMS;

  double distance = 0.0;
  double time = 0.0;
  for (int i = startID; i >= 0 && i < count; i += inc)
  {
    double const len = points[i - inc].Length(points[i]);
    distance += len;
    time += len / speed;

    TurnsMapT::const_iterator j = m_turns.find(points[i]);
    if (j != m_turns.end())
    {
      vector<PossibleTurn> const & vec = j->second;
      for (size_t k = 0; k < vec.size(); ++k)
      {
        if (vec[k].m_pos.GetFeatureId() != pos.GetFeatureId() ||
            vec[k].m_pos.IsForward() == pos.IsForward())
        {
          PossibleTurn t = vec[k];

          t.m_metersCovered = distance;
          t.m_secondsCovered = time;
          turns.push_back(t);
        }
      }
    }
  }
}

double RoadGraphMockSource::GetFeatureDistance(RoadPos const & p1, RoadPos const & p2)
{
  return 0.0;
}

void RoadGraphMockSource::ReconstructPath(RoadPosVectorT const & positions, PointsVectorT & poly)
{

}

} // namespace routing_test
