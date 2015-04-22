#pragma once

#include "routing/road_graph.hpp"

namespace routing_test
{
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
  void AddRoad(RoadInfo && ri);

  // routing::IRoadGraph overrides:
  RoadInfo GetRoadInfo(uint32_t featureId) override;
  double GetSpeedKMPH(uint32_t featureId) override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    CrossTurnsLoader & turnsLoader) override;
};

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & graphMock);
void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graphMock);

}  // namespace routing_test
