#include "road_graph_builder.hpp"

#include "../../base/logging.hpp"

#include "../../std/algorithm.hpp"

using namespace routing;

namespace routing_test
{
void RoadGraphMockSource::AddRoad(RoadInfo && ri)
{
  /// @todo Do CHECK for RoadInfo params.
  uint32_t const roadId = m_roads.size();

  CHECK_GREATER_OR_EQUAL(ri.m_points.size(), 2, ("Empty road"));
  size_t const numSegments = ri.m_points.size() - 1;

  for (size_t segId = 0; segId < numSegments; ++segId)
  {
    PossibleTurn t;
    t.m_startPoint = ri.m_points.front();
    t.m_endPoint = ri.m_points.back();
    t.m_speedKMPH = ri.m_speedKMPH;

    t.m_pos = RoadPos(roadId, true /* forward */, segId, ri.m_points[segId + 1] /* segEndPoint */);
    m_turns[t.m_pos.GetSegEndpoint()].push_back(t);
    if (ri.m_bidirectional)
    {
      t.m_pos = RoadPos(roadId, false /* forward */, segId, ri.m_points[segId] /* segEndPoint */);
      m_turns[t.m_pos.GetSegEndpoint()].push_back(t);
    }
  }

  m_roads.push_back(move(ri));
}

IRoadGraph::RoadInfo RoadGraphMockSource::GetRoadInfo(uint32_t featureId)
{
  CHECK_LESS(featureId, m_roads.size(), ("Invalid feature id."));
  return m_roads[featureId];
}

double RoadGraphMockSource::GetSpeedKMPH(uint32_t featureId)
{
  CHECK_LESS(featureId, m_roads.size(), ("Invalid feature id."));
  return m_roads[featureId].m_speedKMPH;
}

void RoadGraphMockSource::ForEachFeatureClosestToCross(m2::PointD const & /* cross */,
                                                       CrossTurnsLoader & turnsLoader)
{
  for (size_t roadId = 0; roadId < m_roads.size(); ++roadId)
    turnsLoader(roadId, m_roads[roadId]);
}

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & src)
{
  IRoadGraph::RoadInfo ri0;
  ri0.m_bidirectional = true;
  ri0.m_speedKMPH = 40;
  ri0.m_points.push_back(m2::PointD(0, 0));
  ri0.m_points.push_back(m2::PointD(5, 0));
  ri0.m_points.push_back(m2::PointD(10, 0));
  ri0.m_points.push_back(m2::PointD(15, 0));
  ri0.m_points.push_back(m2::PointD(20, 0));

  IRoadGraph::RoadInfo ri1;
  ri1.m_bidirectional = true;
  ri1.m_speedKMPH = 40;
  ri1.m_points.push_back(m2::PointD(10, -10));
  ri1.m_points.push_back(m2::PointD(10, -5));
  ri1.m_points.push_back(m2::PointD(10, 0));
  ri1.m_points.push_back(m2::PointD(10, 5));
  ri1.m_points.push_back(m2::PointD(10, 10));

  IRoadGraph::RoadInfo ri2;
  ri2.m_bidirectional = true;
  ri2.m_speedKMPH = 40;
  ri2.m_points.push_back(m2::PointD(15, -5));
  ri2.m_points.push_back(m2::PointD(15, 0));

  IRoadGraph::RoadInfo ri3;
  ri3.m_bidirectional = true;
  ri3.m_speedKMPH = 40;
  ri3.m_points.push_back(m2::PointD(20, 0));
  ri3.m_points.push_back(m2::PointD(25, 5));
  ri3.m_points.push_back(m2::PointD(15, 5));
  ri3.m_points.push_back(m2::PointD(20, 0));

  src.AddRoad(move(ri0));
  src.AddRoad(move(ri1));
  src.AddRoad(move(ri2));
  src.AddRoad(move(ri3));
}

void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graph)
{
  IRoadGraph::RoadInfo ri0;
  ri0.m_bidirectional = true;
  ri0.m_speedKMPH = 40;
  ri0.m_points.push_back(m2::PointD(0, 0));
  ri0.m_points.push_back(m2::PointD(10, 0));
  ri0.m_points.push_back(m2::PointD(25, 0));
  ri0.m_points.push_back(m2::PointD(35, 0));
  ri0.m_points.push_back(m2::PointD(70, 0));
  ri0.m_points.push_back(m2::PointD(80, 0));

  IRoadGraph::RoadInfo ri1;
  ri1.m_bidirectional = true;
  ri1.m_speedKMPH = 40;
  ri1.m_points.push_back(m2::PointD(0, 0));
  ri1.m_points.push_back(m2::PointD(5, 10));
  ri1.m_points.push_back(m2::PointD(5, 40));

  IRoadGraph::RoadInfo ri2;
  ri2.m_bidirectional = true;
  ri2.m_speedKMPH = 40;
  ri2.m_points.push_back(m2::PointD(12, 25));
  ri2.m_points.push_back(m2::PointD(10, 10));
  ri2.m_points.push_back(m2::PointD(10, 0));

  IRoadGraph::RoadInfo ri3;
  ri3.m_bidirectional = true;
  ri3.m_speedKMPH = 40;
  ri3.m_points.push_back(m2::PointD(5, 10));
  ri3.m_points.push_back(m2::PointD(10, 10));
  ri3.m_points.push_back(m2::PointD(70, 10));
  ri3.m_points.push_back(m2::PointD(80, 10));

  IRoadGraph::RoadInfo ri4;
  ri4.m_bidirectional = true;
  ri4.m_speedKMPH = 40;
  ri4.m_points.push_back(m2::PointD(25, 0));
  ri4.m_points.push_back(m2::PointD(27, 25));

  IRoadGraph::RoadInfo ri5;
  ri5.m_bidirectional = true;
  ri5.m_speedKMPH = 40;
  ri5.m_points.push_back(m2::PointD(35, 0));
  ri5.m_points.push_back(m2::PointD(37, 30));
  ri5.m_points.push_back(m2::PointD(70, 30));
  ri5.m_points.push_back(m2::PointD(80, 30));

  IRoadGraph::RoadInfo ri6;
  ri6.m_bidirectional = true;
  ri6.m_speedKMPH = 40;
  ri6.m_points.push_back(m2::PointD(70, 0));
  ri6.m_points.push_back(m2::PointD(70, 10));
  ri6.m_points.push_back(m2::PointD(70, 30));

  IRoadGraph::RoadInfo ri7;
  ri7.m_bidirectional = true;
  ri7.m_speedKMPH = 40;
  ri7.m_points.push_back(m2::PointD(39, 55));
  ri7.m_points.push_back(m2::PointD(80, 55));

  IRoadGraph::RoadInfo ri8;
  ri8.m_bidirectional = true;
  ri8.m_speedKMPH = 40;
  ri8.m_points.push_back(m2::PointD(5, 40));
  ri8.m_points.push_back(m2::PointD(18, 55));
  ri8.m_points.push_back(m2::PointD(39, 55));
  ri8.m_points.push_back(m2::PointD(37, 30));
  ri8.m_points.push_back(m2::PointD(27, 25));
  ri8.m_points.push_back(m2::PointD(12, 25));
  ri8.m_points.push_back(m2::PointD(5, 40));

  graph.AddRoad(move(ri0));
  graph.AddRoad(move(ri1));
  graph.AddRoad(move(ri2));
  graph.AddRoad(move(ri3));
  graph.AddRoad(move(ri4));
  graph.AddRoad(move(ri5));
  graph.AddRoad(move(ri6));
  graph.AddRoad(move(ri7));
  graph.AddRoad(move(ri8));
}

}  // namespace routing_test
