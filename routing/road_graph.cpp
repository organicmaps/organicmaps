#include "routing/road_graph.hpp"

#include "base/assert.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "indexer/mercator.hpp"
#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace routing
{
namespace
{
double TimeBetweenSec(m2::PointD const & v, m2::PointD const & w)
{
  static double const kMaxSpeedMPS = 5000.0 / 3600;
  return MercatorBounds::DistanceOnEarth(v, w) / kMaxSpeedMPS;
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
