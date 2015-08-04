#pragma once

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "std/algorithm.hpp"
#include "std/functional.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/vector.hpp"

namespace routing
{

template <typename TGraph>
class AStarAlgorithm
{
public:
  using TGraphType = TGraph;
  using TVertexType = typename TGraphType::TVertexType;
  using TEdgeType = typename TGraphType::TEdgeType;

  enum class Result
  {
    OK,
    NoPath,
    Cancelled
  };

  friend ostream & operator<<(ostream & os, Result const & result)
  {
    switch (result)
    {
      case Result::OK:
        os << "OK";
        break;
      case Result::NoPath:
        os << "NoPath";
        break;
      case Result::Cancelled:
        os << "Cancelled";
        break;
    }
    return os;
  }

  using TOnVisitedVertexCallback = std::function<void(TVertexType const &, TVertexType const &)>;

  Result FindPath(TGraphType const & graph,
                  TVertexType const & startVertex, TVertexType const & finalVertex,
                  vector<TVertexType> & path,
                  my::Cancellable const & cancellable = my::Cancellable(),
                  TOnVisitedVertexCallback onVisitedVertexCallback = nullptr) const;

  Result FindPathBidirectional(TGraphType const & graph,
                               TVertexType const & startVertex, TVertexType const & finalVertex,
                               vector<TVertexType> & path,
                               my::Cancellable const & cancellable = my::Cancellable(),
                               TOnVisitedVertexCallback onVisitedVertexCallback = nullptr) const;

private:
  // Periodicy of checking is cancellable cancelled.
  static uint32_t constexpr kCancelledPollPeriod = 128;

  // Periodicy of switching a wave of bidirectional algorithm.
  static uint32_t constexpr kQueueSwitchPeriod = 128;

  // Periodicy of calling callback about visited vertice.
  static uint32_t constexpr kVisitedVerticesPeriod = 4;

  // Precision of comparison weights.
  static double constexpr kEpsilon = 1e-6;

  // State is what is going to be put in the priority queue. See the
  // comment for FindPath for more information.
  struct State
  {
    State(TVertexType const & vertex, double distance) : vertex(vertex), distance(distance) {}

    inline bool operator>(State const & rhs) const { return distance > rhs.distance; }

    TVertexType vertex;
    double distance;
  };

  // BidirectionalStepContext keeps all the information that is needed to
  // search starting from one of the two directions. Its main
  // purpose is to make the code that changes directions more readable.
  struct BidirectionalStepContext
  {
    BidirectionalStepContext(bool forward, TVertexType const & startVertex,
                             TVertexType const & finalVertex, TGraphType const & graph)
        : forward(forward), startVertex(startVertex), finalVertex(finalVertex), graph(graph)
    {
      bestVertex = forward ? startVertex : finalVertex;
      pS = ConsistentHeuristic(bestVertex);
    }

    double TopDistance() const
    {
      ASSERT(!queue.empty(), ());
      return bestDistance.at(queue.top().vertex);
    }

    // p_f(v) = 0.5*(π_f(v) - π_r(v)) + 0.5*π_r(t)
    // p_r(v) = 0.5*(π_r(v) - π_f(v)) + 0.5*π_f(s)
    // p_r(v) + p_f(v) = const. Note: this condition is called consistence.
    double ConsistentHeuristic(TVertexType const & v) const
    {
      double const piF = graph.HeuristicCostEstimate(v, finalVertex);
      double const piR = graph.HeuristicCostEstimate(v, startVertex);
      double const piRT = graph.HeuristicCostEstimate(finalVertex, startVertex);
      double const piFS = graph.HeuristicCostEstimate(startVertex, finalVertex);
      if (forward)
      {
        /// @todo careful: with this "return" here and below in the Backward case
        /// the heuristic becomes inconsistent but still seems to work.
        /// return HeuristicCostEstimate(v, finalVertex);
        return 0.5 * (piF - piR + piRT);
      }
      else
      {
        // return HeuristicCostEstimate(v, startVertex);
        return 0.5 * (piR - piF + piFS);
      }
    }

    void GetAdjacencyList(TVertexType const & v, vector<TEdgeType> & adj)
    {
      if (forward)
        graph.GetOutgoingEdgesList(v, adj);
      else
        graph.GetIngoingEdgesList(v, adj);
    }

    bool const forward;
    TVertexType const & startVertex;
    TVertexType const & finalVertex;
    TGraph const & graph;

    priority_queue<State, vector<State>, greater<State>> queue;
    map<TVertexType, double> bestDistance;
    map<TVertexType, TVertexType> parent;
    TVertexType bestVertex;

    double pS;
  };

  static void ReconstructPath(TVertexType const & v, map<TVertexType, TVertexType> const & parent,
                              vector<TVertexType> & path);
  static void ReconstructPathBidirectional(TVertexType const & v, TVertexType const & w,
                                           map<TVertexType, TVertexType> const & parentV,
                                           map<TVertexType, TVertexType> const & parentW,
                                           vector<TVertexType> & path);
};

// This implementation is based on the view that the A* algorithm
// is equivalent to Dijkstra's algorithm that is run on a reweighted
// version of the graph. If an edge (v, w) has length l(v, w), its reduced
// cost is l_r(v, w) = l(v, w) + pi(w) - pi(v), where pi() is any function
// that ensures l_r(v, w) >= 0 for every edge. We set pi() to calculate
// the shortest possible distance to a goal node, and this is a common
// heuristic that people use in A*.
// Refer to these papers for more information:
// http://research.microsoft.com/pubs/154937/soda05.pdf
// http://www.cs.princeton.edu/courses/archive/spr06/cos423/Handouts/EPP%20shortest%20path%20algorithms.pdf

template <typename TGraph>
typename AStarAlgorithm<TGraph>::Result AStarAlgorithm<TGraph>::FindPath(
    TGraphType const & graph,
    TVertexType const & startVertex, TVertexType const & finalVertex,
    vector<TVertexType> & path,
    my::Cancellable const & cancellable,
    TOnVisitedVertexCallback onVisitedVertexCallback) const
{
  if (nullptr == onVisitedVertexCallback)
    onVisitedVertexCallback = [](TVertexType const &, TVertexType const &){};

  map<TVertexType, double> bestDistance;
  priority_queue<State, vector<State>, greater<State>> queue;
  map<TVertexType, TVertexType> parent;

  bestDistance[startVertex] = 0.0;
  queue.push(State(startVertex, 0.0));

  vector<TEdgeType> adj;

  uint32_t steps = 0;
  while (!queue.empty())
  {
    ++steps;

    if (steps % kCancelledPollPeriod == 0 && cancellable.IsCancelled())
      return Result::Cancelled;

    State const stateV = queue.top();
    queue.pop();

    if (stateV.distance > bestDistance[stateV.vertex])
      continue;

    if (steps % kVisitedVerticesPeriod == 0)
      onVisitedVertexCallback(stateV.vertex, finalVertex);

    if (stateV.vertex == finalVertex)
    {
      ReconstructPath(stateV.vertex, parent, path);
      return Result::OK;
    }

    graph.GetOutgoingEdgesList(stateV.vertex, adj);
    for (auto const & edge : adj)
    {
      State stateW(edge.GetTarget(), 0.0);
      if (stateV.vertex == stateW.vertex)
        continue;

      double const len = edge.GetWeight();
      double const piV = graph.HeuristicCostEstimate(stateV.vertex, finalVertex);
      double const piW = graph.HeuristicCostEstimate(stateW.vertex, finalVertex);
      double const reducedLen = len + piW - piV;

      CHECK(reducedLen >= -kEpsilon, ("Invariant violated:", reducedLen, "<", -kEpsilon));
      double const newReducedDist = stateV.distance + max(reducedLen, 0.0);

      auto const t = bestDistance.find(stateW.vertex);
      if (t != bestDistance.end() && newReducedDist >= t->second - kEpsilon)
        continue;

      stateW.distance = newReducedDist;
      bestDistance[stateW.vertex] = newReducedDist;
      parent[stateW.vertex] = stateV.vertex;
      queue.push(stateW);
    }
  }

  return Result::NoPath;
}

template <typename TGraph>
typename AStarAlgorithm<TGraph>::Result AStarAlgorithm<TGraph>::FindPathBidirectional(
    TGraphType const & graph,
    TVertexType const & startVertex, TVertexType const & finalVertex,
    vector<TVertexType> & path,
    my::Cancellable const & cancellable,
    TOnVisitedVertexCallback onVisitedVertexCallback) const
{
  if (nullptr == onVisitedVertexCallback)
    onVisitedVertexCallback = [](TVertexType const &, TVertexType const &){};

  BidirectionalStepContext forward(true /* forward */, startVertex, finalVertex, graph);
  BidirectionalStepContext backward(false /* forward */, startVertex, finalVertex, graph);

  bool foundAnyPath = false;
  double bestPathReducedLength = 0.0;

  forward.bestDistance[startVertex] = 0.0;
  forward.queue.push(State(startVertex, 0.0 /* distance */));

  backward.bestDistance[finalVertex] = 0.0;
  backward.queue.push(State(finalVertex, 0.0 /* distance */));

  // To use the search code both for backward and forward directions
  // we keep the pointers to everything related to the search in the
  // 'current' and 'next' directions. Swapping these pointers indicates
  // changing the end we are searching from.
  BidirectionalStepContext * cur = &forward;
  BidirectionalStepContext * nxt = &backward;

  vector<TEdgeType> adj;

  // It is not necessary to check emptiness for both queues here
  // because if we have not found a path by the time one of the
  // queues is exhausted, we never will.
  uint32_t steps = 0;
  while (!cur->queue.empty() && !nxt->queue.empty())
  {
    ++steps;

    if (steps % kCancelledPollPeriod == 0 && cancellable.IsCancelled())
      return Result::Cancelled;

    if (steps % kQueueSwitchPeriod == 0)
      swap(cur, nxt);

    if (foundAnyPath)
    {
      double const curTop = cur->TopDistance();
      double const nxtTop = nxt->TopDistance();

      // The intuition behind this is that we cannot obtain a path shorter
      // than the left side of the inequality because that is how any path we find
      // will look like (see comment for curPathReducedLength below).
      // We do not yet have the proof that we will not miss a good path by doing so.

      // The shortest reduced path corresponds to the shortest real path
      // because the heuristics we use are consistent.
      // It would be a mistake to make a decision based on real path lengths because
      // several top states in a priority queue may have equal reduced path lengths and
      // different real path lengths.

      if (curTop + nxtTop >= bestPathReducedLength - kEpsilon)
      {
        ReconstructPathBidirectional(cur->bestVertex, nxt->bestVertex, cur->parent, nxt->parent,
                                     path);
        CHECK(!path.empty(), ());
        if (!cur->forward)
          reverse(path.begin(), path.end());
        return Result::OK;
      }
    }

    State const stateV = cur->queue.top();
    cur->queue.pop();

    if (stateV.distance > cur->bestDistance[stateV.vertex])
      continue;

    if (steps % kVisitedVerticesPeriod == 0)
      onVisitedVertexCallback(stateV.vertex, cur->forward ? cur->finalVertex : cur->startVertex);

    cur->GetAdjacencyList(stateV.vertex, adj);
    for (auto const & edge : adj)
    {
      State stateW(edge.GetTarget(), 0.0);
      if (stateV.vertex == stateW.vertex)
        continue;

      double const len = edge.GetWeight();
      double const pV = cur->ConsistentHeuristic(stateV.vertex);
      double const pW = cur->ConsistentHeuristic(stateW.vertex);
      double const reducedLen = len + pW - pV;

      CHECK(reducedLen >= -kEpsilon, ("Invariant violated:", reducedLen, "<", -kEpsilon));
      double const newReducedDist = stateV.distance + max(reducedLen, 0.0);

      auto const itCur = cur->bestDistance.find(stateW.vertex);
      if (itCur != cur->bestDistance.end() && newReducedDist >= itCur->second - kEpsilon)
        continue;

      auto const itNxt = nxt->bestDistance.find(stateW.vertex);
      if (itNxt != nxt->bestDistance.end())
      {
        double const distW = itNxt->second;
        // Reduced length that the path we've just found has in the original graph:
        // find the reduced length of the path's parts in the reduced forward and backward graphs.
        double const curPathReducedLength = newReducedDist + distW;
        // No epsilon here: it is ok to overshoot slightly.
        if (!foundAnyPath || bestPathReducedLength > curPathReducedLength)
        {
          bestPathReducedLength = curPathReducedLength;
          foundAnyPath = true;
          cur->bestVertex = stateV.vertex;
          nxt->bestVertex = stateW.vertex;
        }
      }

      stateW.distance = newReducedDist;
      cur->bestDistance[stateW.vertex] = newReducedDist;
      cur->parent[stateW.vertex] = stateV.vertex;
      cur->queue.push(stateW);
    }
  }

  return Result::NoPath;
}

// static
template <typename TGraph>
void AStarAlgorithm<TGraph>::ReconstructPath(TVertexType const & v,
                                             map<TVertexType, TVertexType> const & parent,
                                             vector<TVertexType> & path)
{
  path.clear();
  TVertexType cur = v;
  while (true)
  {
    path.push_back(cur);
    auto it = parent.find(cur);
    if (it == parent.end())
      break;
    cur = it->second;
  }
  reverse(path.begin(), path.end());
}

// static
template <typename TGraph>
void AStarAlgorithm<TGraph>::ReconstructPathBidirectional(
    TVertexType const & v, TVertexType const & w, map<TVertexType, TVertexType> const & parentV,
    map<TVertexType, TVertexType> const & parentW, vector<TVertexType> & path)
{
  vector<TVertexType> pathV;
  ReconstructPath(v, parentV, pathV);
  vector<TVertexType> pathW;
  ReconstructPath(w, parentW, pathW);
  path.clear();
  path.reserve(pathV.size() + pathW.size());
  path.insert(path.end(), pathV.begin(), pathV.end());
  path.insert(path.end(), pathW.rbegin(), pathW.rend());
}

}  // namespace routing
