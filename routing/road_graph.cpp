#include "routing/road_graph.hpp"

#include "routing/route.hpp"

#include "indexer/mercator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/assert.hpp"

#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace routing
{
namespace
{
double const KMPH2MPS = 1000.0 / (60 * 60);
double const MAX_SPEED_MPS = 5000.0 / (60 * 60);

double CalcDistanceMeters(m2::PointD const & p1, m2::PointD const & p2)
{
  return MercatorBounds::DistanceOnEarth(p1, p2);
}

double TimeBetweenSec(m2::PointD const & p1, m2::PointD const & p2)
{
  return CalcDistanceMeters(p1, p2) / MAX_SPEED_MPS;
}
}  // namespace

RoadPos::RoadPos(uint32_t featureId, bool forward, size_t segId, m2::PointD const & p)
    : m_featureId((featureId << 1) + (forward ? 1 : 0)), m_segId(segId), m_segEndpoint(p)
{
  ASSERT_LESS(featureId, 1U << 31, ());
  ASSERT_LESS(segId, numeric_limits<uint32_t>::max(), ());
}

string DebugPrint(RoadPos const & r)
{
  ostringstream ss;
  ss << "{ featureId: " << r.GetFeatureId() << ", isForward: " << r.IsForward()
     << ", segId:" << r.m_segId << ", segEndpoint:" << DebugPrint(r.m_segEndpoint) << "}";
  return ss.str();
}

// IRoadGraph ------------------------------------------------------------------

void IRoadGraph::ReconstructPath(RoadPosVectorT const & positions, Route & route)
{
  CHECK(!positions.empty(), ("Can't reconstruct path from an empty list of positions."));

  vector<m2::PointD> path;
  path.reserve(positions.size());

  double trackTimeSec = 0.0;
  m2::PointD prevPoint = positions[0].GetSegEndpoint();
  path.push_back(prevPoint);
  for (size_t i = 1; i < positions.size(); ++i)
  {
    m2::PointD curPoint = positions[i].GetSegEndpoint();

    // By some reason there're two adjacent positions on a road with
    // the same end-points. This could happen, for example, when
    // direction on a road was changed.  But it doesn't matter since
    // this code reconstructs only geometry of a route.
    if (curPoint == prevPoint)
      continue;

    path.push_back(curPoint);

    double const lengthM = CalcDistanceMeters(prevPoint, curPoint);
    trackTimeSec += lengthM / (GetSpeedKMPH(positions[i - 1].GetFeatureId()) * KMPH2MPS);
    prevPoint = curPoint;
  }

  if (path.size() == 1)
  {
    m2::PointD point = path.front();
    path.push_back(point);
  }

  ASSERT_GREATER_OR_EQUAL(path.size(), 2, ());

  // TODO: investigate whether it worth to reconstruct detailed turns
  // and times.
  Route::TimesT times;
  times.emplace_back(path.size() - 1, trackTimeSec);

  Route::TurnsT turnsDir;
  turnsDir.emplace_back(path.size() - 1, turns::ReachedYourDestination);

  route.SetGeometry(path.begin(), path.end());
  route.SetTurnInstructions(turnsDir);
  route.SetSectionTimes(times);
}

// RoadGraph -------------------------------------------------------------------

RoadGraph::RoadGraph(IRoadGraph & roadGraph) : m_roadGraph(roadGraph) {}

void RoadGraph::GetAdjacencyListImpl(RoadPos const & v, vector<RoadEdge> & adj) const
{
  IRoadGraph::TurnsVectorT turns;
  m_roadGraph.GetNearestTurns(v, turns);
  for (PossibleTurn const & turn : turns)
  {
    RoadPos const & w = turn.m_pos;
    adj.emplace_back(w, TimeBetweenSec(v.GetSegEndpoint(), w.GetSegEndpoint()));
  }
}

double RoadGraph::HeuristicCostEstimateImpl(RoadPos const & v, RoadPos const & w) const
{
  return TimeBetweenSec(v.GetSegEndpoint(), w.GetSegEndpoint());
}

}  // namespace routing
