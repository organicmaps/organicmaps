#include "astar_router.hpp"

#include "../indexer/mercator.hpp"
#include "../geometry/distance_on_sphere.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../std/algorithm.hpp"

namespace routing
{
static double const kMaxSpeedMPS = 5000.0 / 3600;
static double const kEpsilon = 1e-6;

namespace
{
template <typename E>
void SortUnique(vector<E> & v)
{
  sort(v.begin(), v.end());
  v.resize(unique(v.begin(), v.end()) - v.begin());
}

template <typename E>
bool Contains(vector<E> const & v, E const & e)
{
  return binary_search(v.begin(), v.end(), e);
}

void ReconstructRoute(RoadPos const & v, map<RoadPos, RoadPos> const & parent,
                      vector<RoadPos> & route)
{
  route.clear();
  RoadPos cur = v;
  LOG(LDEBUG, ("A-star has found a path:"));
  while (true)
  {
    LOG(LDEBUG, (cur));
    route.push_back(cur);
    auto it = parent.find(cur);
    if (it == parent.end())
      break;
    cur = it->second;
  }
}

void ReconstructRouteBidirectional(RoadPos const & v, RoadPos const & w,
                                   map<RoadPos, RoadPos> const & parentV,
                                   map<RoadPos, RoadPos> const & parentW, vector<RoadPos> & route)
{
  vector<RoadPos> routeV;
  ReconstructRoute(v, parentV, routeV);
  vector<RoadPos> routeW;
  ReconstructRoute(w, parentW, routeW);
  route.insert(route.end(), routeV.rbegin(), routeV.rend());
  route.insert(route.end(), routeW.begin(), routeW.end());
}

double HeuristicCostEstimate(RoadPos const & p, vector<RoadPos> const & goals)
{
  // @todo support of more than one goal
  ASSERT(!goals.empty(), ());

  m2::PointD const & b = goals[0].GetSegEndpoint();
  m2::PointD const & e = p.GetSegEndpoint();

  return MercatorBounds::DistanceOnEarth(b, e) / kMaxSpeedMPS;
}

double DistanceBetween(RoadPos const & p1, RoadPos const & p2)
{
  m2::PointD const & b = p1.GetSegEndpoint();
  m2::PointD const & e = p2.GetSegEndpoint();
  return MercatorBounds::DistanceOnEarth(b, e);
}

// Vertex is what is going to be put in the priority queue. See the comment
// for CalculateRouteM2M for more information.
struct Vertex
{
  Vertex(RoadPos const & pos, double dist) : pos(pos), dist(dist) {}

  bool operator<(Vertex const & v) const { return dist > v.dist; }

  RoadPos pos;
  double dist;
};
}  // namespace

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
IRouter::ResultCode AStarRouter::CalculateRouteM2M(vector<RoadPos> const & startPos,
                                                   vector<RoadPos> const & finalPos,
                                                   vector<RoadPos> & route)
{
  ASSERT(!startPos.empty(), ());
  ASSERT(!finalPos.empty(), ());
#if defined(DEBUG)
  for (auto const & roadPos : startPos)
    LOG(LDEBUG, ("AStarRouter::CalculateM2MRoute(): startPos:", roadPos));
  for (auto const & roadPos : finalPos)
    LOG(LDEBUG, ("AStarRouter::CalculateM2MRoute(): finalPos:", roadPos));
#endif  // defined(DEBUG)

  route.clear();
  vector<uint32_t> sortedStartFeatures(startPos.size());
  for (size_t i = 0; i < startPos.size(); ++i)
    sortedStartFeatures[i] = startPos[i].GetFeatureId();
  SortUnique(sortedStartFeatures);

  vector<RoadPos> sortedStartPos(startPos.begin(), startPos.end());
  sort(sortedStartPos.begin(), sortedStartPos.end());

  map<RoadPos, double> bestDistance;
  priority_queue<Vertex> queue;
  map<RoadPos, RoadPos> parent;
  for (auto const & rp : finalPos)
  {
    VERIFY(bestDistance.insert({rp, 0.0}).second, ());
    queue.push(Vertex(rp, 0.0));
  }

  while (!queue.empty())
  {
    Vertex const v = queue.top();
    queue.pop();

    if (v.dist > bestDistance[v.pos])
      continue;

    if (Contains(sortedStartPos, v.pos))
    {
      ReconstructRoute(v.pos, parent, route);
      return IRouter::NoError;
    }

    IRoadGraph::TurnsVectorT turns;
    m_roadGraph->GetNearestTurns(v.pos, turns);
    for (PossibleTurn const & turn : turns)
    {
      ASSERT_GREATER(turn.m_speed, 0.0, ()); /// @todo why?
      Vertex w(turn.m_pos, 0.0);
      if (v.pos == w.pos)
        continue;
      double const len = DistanceBetween(v.pos, w.pos) / kMaxSpeedMPS;
      double const piV = HeuristicCostEstimate(v.pos, sortedStartPos);
      double const piW = HeuristicCostEstimate(w.pos, sortedStartPos);
      double reducedLen = len + piW - piV;
      // It is possible (and in fact happens often when both starting and ending
      // points are on the same highway) that parent links form infinite
      // loops because of precision errors that occur when the reduced edge length is
      // close to zero. That is why kEpsilon is here.
      ASSERT(reducedLen >= -kEpsilon, ("Invariant violation!"));
      double const newReducedDist = v.dist + max(reducedLen, 0.0);

      map<RoadPos, double>::const_iterator t = bestDistance.find(turn.m_pos);
      if (t != bestDistance.end() && newReducedDist >= t->second - kEpsilon)
        continue;

      w.dist = newReducedDist;
      bestDistance[w.pos] = newReducedDist;
      parent[w.pos] = v.pos;
      queue.push(w);
    }
  }

  return IRouter::RouteNotFound;
}

}  // namespace routing
