#pragma once

#include "../road_graph.hpp"

namespace routing_test
{
/// This struct represents road as a broken line.
struct RoadInfo
{
  /// Points of a broken line representing the road.
  vector<m2::PointD> m_points;

  /// Speed on the road.
  double m_speedMS;

  /// True when it's possible to move in both directions on the orad.
  bool m_bidirectional;

  RoadInfo() : m_speedMS(0.0), m_bidirectional(false) {}

  void Swap(RoadInfo & r);
};

class RoadGraphMockSource : public routing::IRoadGraph
{
  vector<RoadInfo> m_roads;

  struct LessPoint
  {
    bool operator()(m2::PointD const & p1, m2::PointD const & p2) const
    {
      double const eps = 1.0E-6;

      if (p1.x + eps < p2.x)
        return true;
      else if (p1.x > p2.x + eps)
        return false;

      return (p1.y + eps < p2.y);
    }
  };

  typedef vector<routing::PossibleTurn> TurnsVectorT;
  typedef map<m2::PointD, TurnsVectorT, LessPoint> TurnsMapT;
  TurnsMapT m_turns;

public:
  void AddRoad(RoadInfo & rd);

  virtual void GetPossibleTurns(routing::RoadPos const & pos, TurnsVectorT & turns,
                                bool noOptimize = true);
  virtual void ReconstructPath(RoadPosVectorT const & positions, routing::Route & route);
};

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & graphMock);
void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graphMock);

}  // namespace routing_test
