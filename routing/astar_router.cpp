#include "astar_router.hpp"

#include "../indexer/mercator.hpp"
#include "../geometry/distance_on_sphere.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../std/algorithm.hpp"


namespace routing
{
static double const kMaxSpeedMPS = 5000.0 / 3600;

// This implementation is based on the view that the A* algorithm
// is equivalent to Dijkstra's algorithm that is run on a reweighted
// version of the graph. If an edge (v, w) has length l(v, w), its reduced
// cost is l_r(v, w) = l(v, w) + pi(w) - pi(v), where pi() is any function
// that ensures l_r(v, w) >= 0 for every edge. We set pi() to calculate
// the shortest possible distance to a goal node, and this is a common
// heuristic that people use in A*.
// Refer to this paper for more information:
// http://research.microsoft.com/pubs/154937/soda05.pdf
//
// The vertices of the graph are of type RoadPos.
// The edges of the graph are of type PossibleTurn.
void AStarRouter::CalculateM2MRoute(vector<RoadPos> const & startPos,
                                    vector<RoadPos> const & finalPos, vector<RoadPos> & route)
{
#if defined(DEBUG)
  for (auto const & roadPos : finalPos)
    LOG(LDEBUG, ("AStarRouter::CalculateM2MRoute(): finalPos:", roadPos));
#endif  // defined(DEBUG)

  ASSERT_GREATER(finalPos.size(), 0, ());
  m_bestDistance.clear();
  for (auto const & roadPos : finalPos)
  {
    VERIFY(m_bestDistance.insert({roadPos, 0.0}).second, ());
  }

#if defined(DEBUG)
  for (auto const & roadPos : startPos)
    LOG(LDEBUG, ("AStarRouter::CalculateM2MRoute(): startPos:", roadPos));
#endif // defined(DEBUG)

  route.clear();
  vector<uint32_t> sortedStartFeatures(startPos.size());
  for (size_t i = 0; i < startPos.size(); ++i)
    sortedStartFeatures[i] = startPos[i].GetFeatureId();
  sort(sortedStartFeatures.begin(), sortedStartFeatures.end());
  sortedStartFeatures.resize(unique(sortedStartFeatures.begin(), sortedStartFeatures.end()) - sortedStartFeatures.begin());

  vector<RoadPos> sortedStartPos(startPos.begin(), startPos.end());
  sort(sortedStartPos.begin(), sortedStartPos.end());

  priority_queue<Vertex> queue;
  for (auto const & p : m_bestDistance)
    queue.push(Vertex(p.first));

  while (!queue.empty())
  {
    Vertex const v = queue.top();
    queue.pop();

    if (v.GetReducedDist() > m_bestDistance[v.GetPos()])
      continue;

    /// @todo why do we even need this?
    bool isStartFeature = binary_search(sortedStartFeatures.begin(), sortedStartFeatures.end(), v.GetPos().GetFeatureId());

    if (isStartFeature && binary_search(sortedStartPos.begin(), sortedStartPos.end(), v.GetPos()))
    {
      ReconstructRoute(v.GetPos(), route);
      return;
    }

    IRoadGraph::TurnsVectorT turns;
    m_pRoadGraph->GetPossibleTurns(v.GetPos(), turns, isStartFeature);
    for (IRoadGraph::TurnsVectorT::const_iterator it = turns.begin(); it != turns.end(); ++it)
    {
      PossibleTurn const & turn = *it;
      ASSERT_GREATER(turn.m_speed, 0.0, ()); /// @todo why?
      Vertex w(turn.m_pos);
      if (v.GetPos() == w.GetPos())
        continue;
      double const len = DistanceBetween(v, w) / kMaxSpeedMPS;
      double const piV = HeuristicCostEstimate(v, sortedStartPos);
      double const piW = HeuristicCostEstimate(w, sortedStartPos);
      double const newReducedDist = v.GetReducedDist() + len + piW - piV;

      RoadPosToDoubleMapT::const_iterator t = m_bestDistance.find(turn.m_pos);
      if (t != m_bestDistance.end() && newReducedDist >= t->second)
        continue;

      w.SetReducedDist(newReducedDist);
      m_bestDistance[w.GetPos()] = newReducedDist;
      m_parent[w.GetPos()] = v.GetPos();
      queue.push(w);
    }
  }

  LOG(LDEBUG, ("No route found!"));
}

double AStarRouter::HeuristicCostEstimate(Vertex const & v, vector<RoadPos> const & goals)
{
  // @todo support of more than one goal
  ASSERT_GREATER(goals.size(), 0, ());

  m2::PointD const & b = goals[0].GetSegEndpoint();
  m2::PointD const & e = v.GetPos().GetSegEndpoint();

  return MercatorBounds::DistanceOnEarth(b, e) / kMaxSpeedMPS;
}

double AStarRouter::DistanceBetween(Vertex const & v1, Vertex const & v2)
{
  m2::PointD const & b = v1.GetPos().GetSegEndpoint();
  m2::PointD const & e = v2.GetPos().GetSegEndpoint();
  return MercatorBounds::DistanceOnEarth(b, e);
}

void AStarRouter::ReconstructRoute(RoadPos const & destination, vector<RoadPos> & route) const
{
  RoadPos rp = destination;

  LOG(LDEBUG, ("A-star has found a route"));
  while (true)
  {
    route.push_back(rp);
    RoadPosParentMapT::const_iterator it = m_parent.find(rp);
    if (it == m_parent.end())
      break;
    rp = it->second;
  }
}

}
