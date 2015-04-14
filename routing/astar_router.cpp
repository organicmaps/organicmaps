#include "routing/astar_router.hpp"

#include "indexer/mercator.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/algorithm.hpp"

namespace routing
{
double const kMaxSpeedMPS = 5000.0 / 3600;
double const kEpsilon = 1e-6;

// Power of two to make the division faster.
uint32_t const kCancelledPollPeriod = 128;
uint32_t const kQueueSwitchPeriod = 128;

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

    // We need the original start position because it contains the projection point to the road
    // feature.
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

  route.clear();

  vector<RoadPos> sortedStartPos(startPos.begin(), startPos.end());
  sort(sortedStartPos.begin(), sortedStartPos.end());

  vector<RoadPos> sortedFinalPos(finalPos.begin(), finalPos.end());
  sort(sortedFinalPos.begin(), sortedFinalPos.end());

  priority_queue<Vertex> queueForward;
  priority_queue<Vertex> queueBackward;
  map<RoadPos, double> bestDistanceForward;
  map<RoadPos, double> bestDistanceBackward;
  map<RoadPos, RoadPos> parentForward;
  map<RoadPos, RoadPos> parentBackward;
  for (auto const & rp : startPos)
  {
    VERIFY(bestDistanceForward.insert({rp, 0.0}).second, ());
    queueForward.push(Vertex(rp, 0.0 /* distance */));
  }
  for (auto const & rp : finalPos)
  {
    VERIFY(bestDistanceBackward.insert({rp, 0.0}).second, ());
    queueBackward.push(Vertex(rp, 0.0 /* distance */));
  }

  bool curForward = true;
  priority_queue<Vertex> * curQueue = &queueForward;
  priority_queue<Vertex> * nxtQueue = &queueBackward;
  map<RoadPos, double> * curBestDistance = &bestDistanceForward;
  map<RoadPos, double> * nxtBestDistance = &bestDistanceBackward;
  map<RoadPos, RoadPos> * curParent = &parentForward;
  map<RoadPos, RoadPos> * nxtParent = &parentBackward;
  vector<RoadPos> * curSortedPos = &sortedStartPos;
  vector<RoadPos> * nxtSortedPos = &sortedFinalPos;
  Vertex curBest(startPos[0], 0.0 /* distance */);
  Vertex nxtBest(finalPos[0], 0.0 /* distance */);
  bool foundAnyPath = false;
  double bestPathLength = 0.0;

  // p_f(v) = 0.5*(pi_f(v) - pi_r(v)) + 0.5*pi_r(t)
  // p_r(v) = 0.5*(pi_r(v) - pi_f(v)) + 0.5*pi_f(s)
  // p_r(v) + p_f(v) = const. Note: this condition is called consistence.
  auto consistentHeuristicForward = [&](Vertex const & v) -> double
  {
    /// @todo careful: with this "return" here and below in the Backward case
    /// the heuristic becomes inconsistent but still seems to work.
    // return HeuristicCostEstimate(v.pos, finalPos);
    static double const piRT = HeuristicCostEstimate(finalPos[0], startPos);
    double piF = HeuristicCostEstimate(v.pos, finalPos);
    double piR = HeuristicCostEstimate(v.pos, startPos);
    return 0.5 * (piF - piR + piRT);
  };
  auto consistentHeuristicBackward = [&](Vertex const & v) -> double
  {
    // return HeuristicCostEstimate(v.pos, startPos);
    static double const piS = HeuristicCostEstimate(startPos[0], finalPos);
    double piF = HeuristicCostEstimate(v.pos, finalPos);
    double piR = HeuristicCostEstimate(v.pos, startPos);
    return 0.5 * (piR - piF + piS);
  };

  double pS = consistentHeuristicForward(Vertex(startPos[0], 0.0));
  double pT = consistentHeuristicBackward(Vertex(finalPos[0], 0.0));

  // It is not necessary to check emptiness for both queues here
  // because if we have not found a path by the time one of the
  // queues is exhausted, we never will.
  uint32_t steps = 0;
  while (!curQueue->empty() && !nxtQueue->empty())
  {
    ++steps;

    if (steps % kCancelledPollPeriod == 0)
    {
      if (IsCancelled())
        return IRouter::Cancelled;
    }

    if (steps % kQueueSwitchPeriod == 0)
    {
      curForward ^= true;
      swap(curQueue, nxtQueue);
      swap(curBestDistance, nxtBestDistance);
      swap(curParent, nxtParent);
      swap(curSortedPos, nxtSortedPos);
      swap(curBest, nxtBest);
      swap(pS, pT);
    }

    double curTop = (*curBestDistance)[curQueue->top().pos];
    double nxtTop = (*nxtBestDistance)[nxtQueue->top().pos];

    double pTopV = (curForward ? consistentHeuristicForward(curQueue->top())
                               : consistentHeuristicBackward(curQueue->top()));
    double pTopW = (curForward ? consistentHeuristicBackward(nxtQueue->top())
                               : consistentHeuristicForward(nxtQueue->top()));

    if (foundAnyPath && curTop + nxtTop - pTopV + pS - pTopW + pT >= bestPathLength - kEpsilon)
    {
      ReconstructRouteBidirectional(curBest.pos, nxtBest.pos, *curParent, *nxtParent, route);
      CHECK_GREATER(route.size(), 1U, ());
      if (!Contains(sortedFinalPos, route[0]))
        reverse(route.begin(), route.end());
      ASSERT(Contains(sortedFinalPos, route[0]), ());
      ASSERT(Contains(sortedStartPos, route.back()), ());
      LOG(LINFO, ("bidirectional steps made:", steps));
      return IRouter::NoError;
    }

    Vertex const v = curQueue->top();
    curQueue->pop();

    if (v.dist > (*curBestDistance)[v.pos])
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
      double const pV =
          (curForward ? consistentHeuristicForward(v) : consistentHeuristicBackward(v));
      double const pW =
          (curForward ? consistentHeuristicForward(w) : consistentHeuristicBackward(w));
      double const reducedLen = len + pW - pV;
      double const pRW =
          (curForward ? consistentHeuristicBackward(w) : consistentHeuristicForward(w));
      CHECK(reducedLen >= -kEpsilon, ("Invariant failed:", (len + pW - pV), "<", -kEpsilon));
      double newReducedDist = v.dist + max(reducedLen, 0.0);

      map<RoadPos, double>::const_iterator t = curBestDistance->find(w.pos);
      if (t != curBestDistance->end() && newReducedDist >= t->second - kEpsilon)
        continue;

      if (nxtBestDistance->find(w.pos) != nxtBestDistance->end())
      {
        double const distW = (*nxtBestDistance)[w.pos];
        double const curPathLength = newReducedDist + distW - pW + pS - pRW + pT;
        if (!foundAnyPath || bestPathLength > curPathLength)
        {
          bestPathLength = curPathLength;
          foundAnyPath = true;
          curBest = v;
          nxtBest = w;
        }
      }

      w.dist = newReducedDist;
      (*curBestDistance)[w.pos] = newReducedDist;
      (*curParent)[w.pos] = v.pos;
      curQueue->push(w);
    }
  }

  LOG(LDEBUG, ("No route found!"));
  return IRouter::RouteNotFound;
}

}  // namespace routing
