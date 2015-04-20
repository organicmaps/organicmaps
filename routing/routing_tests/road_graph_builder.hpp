#pragma once

#include "routing/road_graph.hpp"

namespace routing_test
{
/// This struct represents road as a polyline.
struct RoadInfo
{
  /// Points of a polyline representing the road.
  vector<m2::PointD> m_points;

  /// Speed on the road.
  double m_speedMS;

  /// Indicates whether the road is bidirectional.
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

  // routing::IRoadGraph overrides:
  void GetNearestTurns(routing::RoadPos const & pos, TurnsVectorT & turns) override;
  double GetSpeedKMPH(uint32_t featureId) override;
};

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & graphMock);
void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graphMock);

}  // namespace routing_test
