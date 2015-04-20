#include "road_graph_builder.hpp"

#include "../../base/logging.hpp"

#include "../../std/algorithm.hpp"

using namespace routing;

namespace routing_test
{
double const MPS2KMPH = (60 * 60) / 1000.0;

void RoadInfo::Swap(RoadInfo & r)
{
  m_points.swap(r.m_points);
  std::swap(m_speedMS, r.m_speedMS);
  std::swap(m_bidirectional, r.m_bidirectional);
}

void RoadGraphMockSource::AddRoad(RoadInfo & rd)
{
  /// @todo Do CHECK for RoadInfo params.
  uint32_t const roadId = m_roads.size();

  CHECK_GREATER_OR_EQUAL(rd.m_points.size(), 2, ("Empty road"));
  size_t const numSegments = rd.m_points.size() - 1;

  for (size_t segId = 0; segId < numSegments; ++segId)
  {
    PossibleTurn t;
    t.m_startPoint = rd.m_points.front();
    t.m_endPoint = rd.m_points.back();
    t.m_speed = rd.m_speedMS;

    t.m_pos = RoadPos(roadId, true /* forward */, segId, rd.m_points[segId + 1] /* segEndPoint */);
    m_turns[t.m_pos.GetSegEndpoint()].push_back(t);
    if (rd.m_bidirectional)
    {
      t.m_pos = RoadPos(roadId, false /* forward */, segId, rd.m_points[segId] /* segEndPoint */);
      m_turns[t.m_pos.GetSegEndpoint()].push_back(t);
    }
  }

  m_roads.push_back(RoadInfo());
  m_roads.back().Swap(rd);
}

void RoadGraphMockSource::GetNearestTurns(RoadPos const & pos, TurnsVectorT & turns)
{
  // TODO (@gorshenin): this partially duplicates code in
  // CrossFeaturesLoader. Possible solution is to make
  // CrossFeaturesLoader abstract enough to be used here and in
  // FeaturesRoadGraph.

  CHECK_LESS(pos.GetFeatureId(), m_roads.size(), ("Invalid feature id."));
  RoadInfo const & curRoad = m_roads.at(pos.GetFeatureId());

  CHECK_LESS(pos.GetSegStartPointId(), curRoad.m_points.size(), ("Invalid point id."));
  m2::PointD const curPoint = curRoad.m_points[pos.GetSegStartPointId()];

  for (size_t featureId = 0; featureId < m_roads.size(); ++featureId)
  {
    RoadInfo const & road = m_roads[featureId];
    vector<m2::PointD> const & points = road.m_points;
    if (road.m_speedMS <= 0.0)
      continue;
    PossibleTurn turn;
    turn.m_startPoint = points.front();
    turn.m_endPoint = points.back();
    turn.m_speed = road.m_speedMS;
    for (size_t i = 0; i < points.size(); ++i)
    {
      m2::PointD point = points[i];
      if (!m2::AlmostEqual(curPoint, point))
        continue;
      if (i > 0)
      {
        turn.m_pos = RoadPos(featureId, true /* forward */, i - 1, point);
        turns.push_back(turn);
      }

      if (i + 1 < points.size())
      {
        turn.m_pos = RoadPos(featureId, false /* forward */, i, point);
        turns.push_back(turn);
      }
    }
  }
}

double RoadGraphMockSource::GetSpeedKMPH(uint32_t featureId)
{
  CHECK_LESS(featureId, m_roads.size(), ("Invalid feature id."));
  return m_roads[featureId].m_speedMS * MPS2KMPH;
}

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & src)
{
  RoadInfo ri0;
  ri0.m_bidirectional = true;
  ri0.m_speedMS = 40;
  ri0.m_points.push_back(m2::PointD(0, 0));
  ri0.m_points.push_back(m2::PointD(5, 0));
  ri0.m_points.push_back(m2::PointD(10, 0));
  ri0.m_points.push_back(m2::PointD(15, 0));
  ri0.m_points.push_back(m2::PointD(20, 0));

  RoadInfo ri1;
  ri1.m_bidirectional = true;
  ri1.m_speedMS = 40;
  ri1.m_points.push_back(m2::PointD(10, -10));
  ri1.m_points.push_back(m2::PointD(10, -5));
  ri1.m_points.push_back(m2::PointD(10, 0));
  ri1.m_points.push_back(m2::PointD(10, 5));
  ri1.m_points.push_back(m2::PointD(10, 10));

  RoadInfo ri2;
  ri2.m_bidirectional = true;
  ri2.m_speedMS = 40;
  ri2.m_points.push_back(m2::PointD(15, -5));
  ri2.m_points.push_back(m2::PointD(15, 0));

  RoadInfo ri3;
  ri3.m_bidirectional = true;
  ri3.m_speedMS = 40;
  ri3.m_points.push_back(m2::PointD(20, 0));
  ri3.m_points.push_back(m2::PointD(25, 5));
  ri3.m_points.push_back(m2::PointD(15, 5));
  ri3.m_points.push_back(m2::PointD(20, 0));

  src.AddRoad(ri0);
  src.AddRoad(ri1);
  src.AddRoad(ri2);
  src.AddRoad(ri3);
}

void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graph)
{
  RoadInfo ri0;
  ri0.m_bidirectional = true;
  ri0.m_speedMS = 40;
  ri0.m_points.push_back(m2::PointD(0, 0));
  ri0.m_points.push_back(m2::PointD(10, 0));
  ri0.m_points.push_back(m2::PointD(25, 0));
  ri0.m_points.push_back(m2::PointD(35, 0));
  ri0.m_points.push_back(m2::PointD(70, 0));
  ri0.m_points.push_back(m2::PointD(80, 0));

  RoadInfo ri1;
  ri1.m_bidirectional = true;
  ri1.m_speedMS = 40;
  ri1.m_points.push_back(m2::PointD(0, 0));
  ri1.m_points.push_back(m2::PointD(5, 10));
  ri1.m_points.push_back(m2::PointD(5, 40));

  RoadInfo ri2;
  ri2.m_bidirectional = true;
  ri2.m_speedMS = 40;
  ri2.m_points.push_back(m2::PointD(12, 25));
  ri2.m_points.push_back(m2::PointD(10, 10));
  ri2.m_points.push_back(m2::PointD(10, 0));

  RoadInfo ri3;
  ri3.m_bidirectional = true;
  ri3.m_speedMS = 40;
  ri3.m_points.push_back(m2::PointD(5, 10));
  ri3.m_points.push_back(m2::PointD(10, 10));
  ri3.m_points.push_back(m2::PointD(70, 10));
  ri3.m_points.push_back(m2::PointD(80, 10));

  RoadInfo ri4;
  ri4.m_bidirectional = true;
  ri4.m_speedMS = 40;
  ri4.m_points.push_back(m2::PointD(25, 0));
  ri4.m_points.push_back(m2::PointD(27, 25));

  RoadInfo ri5;
  ri5.m_bidirectional = true;
  ri5.m_speedMS = 40;
  ri5.m_points.push_back(m2::PointD(35, 0));
  ri5.m_points.push_back(m2::PointD(37, 30));
  ri5.m_points.push_back(m2::PointD(70, 30));
  ri5.m_points.push_back(m2::PointD(80, 30));

  RoadInfo ri6;
  ri6.m_bidirectional = true;
  ri6.m_speedMS = 40;
  ri6.m_points.push_back(m2::PointD(70, 0));
  ri6.m_points.push_back(m2::PointD(70, 10));
  ri6.m_points.push_back(m2::PointD(70, 30));

  RoadInfo ri7;
  ri7.m_bidirectional = true;
  ri7.m_speedMS = 40;
  ri7.m_points.push_back(m2::PointD(39, 55));
  ri7.m_points.push_back(m2::PointD(80, 55));

  RoadInfo ri8;
  ri8.m_bidirectional = true;
  ri8.m_speedMS = 40;
  ri8.m_points.push_back(m2::PointD(5, 40));
  ri8.m_points.push_back(m2::PointD(18, 55));
  ri8.m_points.push_back(m2::PointD(39, 55));
  ri8.m_points.push_back(m2::PointD(37, 30));
  ri8.m_points.push_back(m2::PointD(27, 25));
  ri8.m_points.push_back(m2::PointD(12, 25));
  ri8.m_points.push_back(m2::PointD(5, 40));

  graph.AddRoad(ri0);
  graph.AddRoad(ri1);
  graph.AddRoad(ri2);
  graph.AddRoad(ri3);
  graph.AddRoad(ri4);
  graph.AddRoad(ri5);
  graph.AddRoad(ri6);
  graph.AddRoad(ri7);
  graph.AddRoad(ri8);
}

}  // namespace routing_test
