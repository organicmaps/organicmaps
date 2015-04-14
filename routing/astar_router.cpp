#include "routing/astar_router.hpp"

#include "indexer/mercator.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/algorithm.hpp"

namespace routing
{
namespace
{
double const kMaxSpeedMPS = 5000.0 / 3600;
double const kEpsilon = 1e-6;

// Power of two to make the division faster.
uint32_t const kCancelledPollPeriod = 128;
uint32_t const kQueueSwitchPeriod = 128;

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
  while (true)
  {
    route.push_back(cur);
    auto it = parent.find(cur);
    if (it == parent.end())
      break;
    cur = it->second;
  }
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

// Vertex is what is going to be put in the priority queue. See the comment
// for CalculateRouteM2M for more information.
struct Vertex
{
  Vertex() : dist(0.0) {}
  Vertex(RoadPos const & pos, double dist) : pos(pos), dist(dist) {}

  bool operator<(Vertex const & v) const { return dist > v.dist; }

  RoadPos pos;
  double dist;
};

// BidirectionalStepContext keeps all the information that is needed to
// search starting from one of the two directions. Its main
// purpose is to make the code that changes directions more readable.
struct BidirectionalStepContext
{
  BidirectionalStepContext(bool forward, vector<RoadPos> const & startPos,
                           vector<RoadPos> const & finalPos)
      : forward(forward), startPos(startPos), finalPos(finalPos)
  {
    bestVertex = forward ? Vertex(startPos[0], 0.0) : Vertex(finalPos[0], 0.0);
    pS = ConsistentHeuristic(Vertex(startPos[0], 0.0));
  }

  double TopDistance() const
  {
    ASSERT(!queue.empty(), ());
    auto it = bestDistance.find(queue.top().pos);
    CHECK(it != bestDistance.end(), ());
    return it->second;
  }

  // p_f(v) = 0.5*(π_f(v) - π_r(v)) + 0.5*π_r(t)
  // p_r(v) = 0.5*(π_r(v) - π_f(v)) + 0.5*π_f(s)
  // p_r(v) + p_f(v) = const. Note: this condition is called consistence.
  double ConsistentHeuristic(Vertex const & v) const
  {
    double piF = HeuristicCostEstimate(v.pos, finalPos);
    double piR = HeuristicCostEstimate(v.pos, startPos);
    double const piRT = HeuristicCostEstimate(finalPos[0], startPos);
    double const piFS = HeuristicCostEstimate(startPos[0], finalPos);
    if (forward)
    {
      /// @todo careful: with this "return" here and below in the Backward case
      /// the heuristic becomes inconsistent but still seems to work.
      /// return HeuristicCostEstimate(v.pos, finalPos);
      return 0.5 * (piF - piR + piRT);
    }
    else
    {
      // return HeuristicCostEstimate(v.pos, startPos);
      return 0.5 * (piR - piF + piFS);
    }
  }

  bool const forward;
  vector<RoadPos> const & startPos;
  vector<RoadPos> const & finalPos;

  priority_queue<Vertex> queue;
  map<RoadPos, double> bestDistance;
  map<RoadPos, RoadPos> parent;
  Vertex bestVertex;

  double pS;
};

}  // namespace

// This implementation is based on the view that the A* algorithm
// is equivalent to Dijkstra's algorithm that is run on a reweighted
// version of the graph. If an edge (v, w) has length l(v, w), its reduced
// cost is l_r(v, w) = l(v, w) + π(w) - π(v), where π() is any function
// that ensures l_r(v, w) >= 0 for every edge. We set π() to calculate
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

  uint32_t steps = 0;

  while (!queue.empty())
  {
    ++steps;
    if (steps % kCancelledPollPeriod == 0)
    {
      if (IsCancelled())
        return IRouter::Cancelled;
    }

    Vertex const v = queue.top();
    queue.pop();

    if (v.dist > bestDistance[v.pos])
      continue;

    // We need the original start position because it contains
    // the projection point to the road feature.
    auto pos = lower_bound(sortedStartPos.begin(), sortedStartPos.end(), v.pos);
    if (pos != sortedStartPos.end() && *pos == v.pos)
    {
      ReconstructRoute(*pos, parent, route);
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

/// @todo This may work incorrectly if (startPos.size() > 1) or (finalPos.size() > 1).
IRouter::ResultCode AStarRouter::CalculateRouteBidirectional(vector<RoadPos> const & startPos,
                                                             vector<RoadPos> const & finalPos,
                                                             vector<RoadPos> & route)
{
  ASSERT(!startPos.empty(), ());
  ASSERT(!finalPos.empty(), ());
#if defined(DEBUG)
  for (auto const & roadPos : startPos)
    LOG(LDEBUG, ("AStarRouter::CalculateRouteBidirectional(): startPos:", roadPos));
  for (auto const & roadPos : finalPos)
    LOG(LDEBUG, ("AStarRouter::CalculateRouteBidirectional(): finalPos:", roadPos));
#endif  // defined(DEBUG)

  BidirectionalStepContext forward(true /* forward */, startPos, finalPos);
  BidirectionalStepContext backward(false /* forward */, startPos, finalPos);

  bool foundAnyPath = false;
  double bestPathLength = 0.0;

  for (auto const & rp : startPos)
  {
    VERIFY(forward.bestDistance.insert({rp, 0.0}).second, ());
    forward.queue.push(Vertex(rp, 0.0 /* distance */));
  }
  for (auto const & rp : finalPos)
  {
    VERIFY(backward.bestDistance.insert({rp, 0.0}).second, ());
    backward.queue.push(Vertex(rp, 0.0 /* distance */));
  }

  // To use the search code both for backward and forward directions
  // we keep the pointers to everything related to the search in the
  // 'current' and 'next' directions. Swapping these pointers indicates
  // changing the end we are searching from.
  BidirectionalStepContext * cur = &forward;
  BidirectionalStepContext * nxt = &backward;

  // It is not necessary to check emptiness for both queues here
  // because if we have not found a path by the time one of the
  // queues is exhausted, we never will.
  uint32_t steps = 0;
  while (!cur->queue.empty() && !nxt->queue.empty())
  {
    ++steps;

    if (steps % kCancelledPollPeriod == 0)
    {
      if (IsCancelled())
        return IRouter::Cancelled;
    }

    if (steps % kQueueSwitchPeriod == 0)
    {
      swap(cur, nxt);
    }

    double const curTop = cur->TopDistance();
    double const nxtTop = nxt->TopDistance();
    double const pTopV = cur->ConsistentHeuristic(cur->queue.top());
    double const pTopW = nxt->ConsistentHeuristic(nxt->queue.top());

    // The intuition behind this is that we cannot obtain a path shorter
    // than the left side of the inequality because that is how any path we find
    // will look like (see comment for curPathLength below).
    // We do not yet have the proof that we will not miss a good path by doing so.
    if (foundAnyPath &&
        curTop + nxtTop - pTopV + cur->pS - pTopW + nxt->pS >= bestPathLength - kEpsilon)
    {
      ReconstructRouteBidirectional(cur->bestVertex.pos, nxt->bestVertex.pos, cur->parent,
                                    nxt->parent, route);
      CHECK(!route.empty(), ());
      if (cur->forward)
        reverse(route.begin(), route.end());
      ASSERT(Contains(sortedFinalPos, route[0]), ());
      ASSERT(Contains(sortedStartPos, route.back()), ());
      LOG(LDEBUG, ("Path found. Bidirectional steps made:", steps, "."));
      return IRouter::NoError;
    }

    Vertex const v = cur->queue.top();
    cur->queue.pop();

    if (v.dist > cur->bestDistance[v.pos])
      continue;

    IRoadGraph::TurnsVectorT turns;
    m_roadGraph->GetNearestTurns(v.pos, turns);
    for (PossibleTurn const & turn : turns)
    {
      ASSERT_GREATER(turn.m_speed, 0.0, ());
      Vertex w(turn.m_pos, 0.0);
      if (v.pos == w.pos)
        continue;
      double const len = DistanceBetween(v.pos, w.pos) / kMaxSpeedMPS;
      double const pV = cur->ConsistentHeuristic(v);
      double const pW = cur->ConsistentHeuristic(w);
      double const reducedLen = len + pW - pV;
      double const pRW = nxt->ConsistentHeuristic(w);
      CHECK(reducedLen >= -kEpsilon, ("Invariant failed:", reducedLen, "<", -kEpsilon));
      double newReducedDist = v.dist + max(reducedLen, 0.0);

      map<RoadPos, double>::const_iterator t = cur->bestDistance.find(w.pos);
      if (t != cur->bestDistance.end() && newReducedDist >= t->second - kEpsilon)
        continue;

      if (nxt->bestDistance.find(w.pos) != nxt->bestDistance.end())
      {
        double const distW = nxt->bestDistance[w.pos];
        // Length that the path we've just found has in the original graph:
        // find the length of the path's parts in the reduced forward and backward
        // graphs and remove the heuristic adjustments.
        double const curPathLength = newReducedDist + distW - pW + cur->pS - pRW + nxt->pS;
        // No epsilon here: it is ok to overshoot slightly.
        if (!foundAnyPath || bestPathLength > curPathLength)
        {
          bestPathLength = curPathLength;
          foundAnyPath = true;
          cur->bestVertex = v;
          nxt->bestVertex = w;
        }
      }

      w.dist = newReducedDist;
      cur->bestDistance[w.pos] = newReducedDist;
      cur->parent[w.pos] = v.pos;
      cur->queue.push(w);
    }
  }

  LOG(LDEBUG, ("No route found!"));
  return IRouter::RouteNotFound;
}

}  // namespace routing
