#pragma once

#include "routing/base/astar_weight.hpp"
#include "routing/base/routing_result.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

namespace routing
{
template <typename Graph>
class AStarAlgorithm
{
public:
  using Vertex = typename Graph::Vertex;
  using Edge = typename Graph::Edge;
  using Weight = typename Graph::Weight;

  enum class Result
  {
    OK,
    NoPath,
    Cancelled
  };

  friend std::ostream & operator<<(std::ostream & os, Result const & result)
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

  using OnVisitedVertexCallback = std::function<void(Vertex const &, Vertex const &)>;
  // Callback used to check path length from start/finish to the edge (including the edge itself)
  // before adding the edge to AStar queue.
  // Can be used to clip some path which does not meet restrictions.
  using CheckLengthCallback = std::function<bool(Weight const &)>;

  struct Params
  {
    Params(Graph & graph, Vertex const & startVertex, Vertex const & finalVertex,
           std::vector<Edge> const * prevRoute, base::Cancellable const & cancellable,
           OnVisitedVertexCallback const & onVisitedVertexCallback,
           CheckLengthCallback const & checkLengthCallback)
      : m_graph(graph)
      , m_startVertex(startVertex)
      , m_finalVertex(finalVertex)
      , m_prevRoute(prevRoute)
      , m_cancellable(cancellable)
      , m_onVisitedVertexCallback(onVisitedVertexCallback ? onVisitedVertexCallback
                                                          : [](Vertex const &, Vertex const &) {})
      , m_checkLengthCallback(checkLengthCallback
                                  ? checkLengthCallback
                                  : [](Weight const & /* weight */) { return true; })
    {
    }

    Graph & m_graph;
    Vertex const m_startVertex;
    // Used for FindPath, FindPathBidirectional.
    Vertex const m_finalVertex;
    // Used for AdjustRoute.
    std::vector<Edge> const * const m_prevRoute;
    base::Cancellable const & m_cancellable;
    OnVisitedVertexCallback const m_onVisitedVertexCallback;
    CheckLengthCallback const m_checkLengthCallback;
  };

  struct ParamsForTests
  {
    ParamsForTests(Graph & graph, Vertex const & startVertex, Vertex const & finalVertex,
                   std::vector<Edge> const * prevRoute,
                   CheckLengthCallback const & checkLengthCallback)
      : m_graph(graph)
      , m_startVertex(startVertex)
      , m_finalVertex(finalVertex)
      , m_prevRoute(prevRoute)
      , m_onVisitedVertexCallback([](Vertex const &, Vertex const &) {})
      , m_checkLengthCallback(checkLengthCallback
                                  ? checkLengthCallback
                                  : [](Weight const & /* weight */) { return true; })
    {
    }

    Graph & m_graph;
    Vertex const m_startVertex;
    // Used for FindPath, FindPathBidirectional.
    Vertex const m_finalVertex;
    // Used for AdjustRoute.
    std::vector<Edge> const * const m_prevRoute;
    base::Cancellable const m_cancellable;
    OnVisitedVertexCallback const m_onVisitedVertexCallback;
    CheckLengthCallback const m_checkLengthCallback;
  };
  class Context final
  {
  public:
    void Clear()
    {
      m_distanceMap.clear();
      m_parents.clear();
    }

    bool HasDistance(Vertex const & vertex) const
    {
      return m_distanceMap.find(vertex) != m_distanceMap.cend();
    }

    Weight GetDistance(Vertex const & vertex) const
    {
      auto const & it = m_distanceMap.find(vertex);
      if (it == m_distanceMap.cend())
        return kInfiniteDistance;

      return it->second;
    }

    void SetDistance(Vertex const & vertex, Weight const & distance)
    {
      m_distanceMap[vertex] = distance;
    }

    void SetParent(Vertex const & parent, Vertex const & child) { m_parents[parent] = child; }

    void ReconstructPath(Vertex const & v, std::vector<Vertex> & path) const;

  private:
    std::map<Vertex, Weight> m_distanceMap;
    std::map<Vertex, Vertex> m_parents;
  };

  // VisitVertex returns true: wave will continue
  // VisitVertex returns false: wave will stop
  template <typename VisitVertex, typename AdjustEdgeWeight, typename FilterStates>
  void PropagateWave(Graph & graph, Vertex const & startVertex, VisitVertex && visitVertex,
                     AdjustEdgeWeight && adjustEdgeWeight, FilterStates && filterStates,
                     Context & context) const;

  template <typename VisitVertex>
  void PropagateWave(Graph & graph, Vertex const & startVertex, VisitVertex && visitVertex,
                     Context & context) const;

  template <typename P>
  Result FindPath(P & params, RoutingResult<Vertex, Weight> & result) const;

  template <typename P>
  Result FindPathBidirectional(P & params, RoutingResult<Vertex, Weight> & result) const;

  // Adjust route to the previous one.
  // Expects |params.m_checkLengthCallback| to check wave propagation limit.
  template <typename P>
  typename AStarAlgorithm<Graph>::Result AdjustRoute(P & params,
                                                     RoutingResult<Vertex, Weight> & result) const;

private:
  // Periodicity of switching a wave of bidirectional algorithm.
  static uint32_t constexpr kQueueSwitchPeriod = 128;

  // Precision of comparison weights.
  static Weight constexpr kEpsilon = GetAStarWeightEpsilon<Weight>();
  static Weight constexpr kZeroDistance = GetAStarWeightZero<Weight>();
  static Weight constexpr kInfiniteDistance = GetAStarWeightMax<Weight>();

  class PeriodicPollCancellable final
  {
  public:
    PeriodicPollCancellable(base::Cancellable const & cancellable) : m_cancellable(cancellable) {}

    bool IsCancelled()
    {
      // Periodicity of checking is cancellable cancelled.
      uint32_t constexpr kPeriod = 128;
      return count++ % kPeriod == 0 && m_cancellable.IsCancelled();
    }

  private:
    base::Cancellable const & m_cancellable;
    uint32_t count = 0;
  };

  // State is what is going to be put in the priority queue. See the
  // comment for FindPath for more information.
  struct State
  {
    State(Vertex const & vertex, Weight const & distance) : vertex(vertex), distance(distance) {}

    inline bool operator>(State const & rhs) const { return distance > rhs.distance; }

    Vertex vertex;
    Weight distance;
  };

  // BidirectionalStepContext keeps all the information that is needed to
  // search starting from one of the two directions. Its main
  // purpose is to make the code that changes directions more readable.
  struct BidirectionalStepContext
  {
    BidirectionalStepContext(bool forward, Vertex const & startVertex, Vertex const & finalVertex,
                             Graph & graph)
      : forward(forward)
      , startVertex(startVertex)
      , finalVertex(finalVertex)
      , graph(graph)
      , m_piRT(graph.HeuristicCostEstimate(finalVertex, startVertex))
      , m_piFS(graph.HeuristicCostEstimate(startVertex, finalVertex))
    {
      bestVertex = forward ? startVertex : finalVertex;
      pS = ConsistentHeuristic(bestVertex);
    }

    Weight TopDistance() const
    {
      ASSERT(!queue.empty(), ());
      return bestDistance.at(queue.top().vertex);
    }

    // p_f(v) = 0.5*(π_f(v) - π_r(v)) + 0.5*π_r(t)
    // p_r(v) = 0.5*(π_r(v) - π_f(v)) + 0.5*π_f(s)
    // p_r(v) + p_f(v) = const. Note: this condition is called consistence.
    Weight ConsistentHeuristic(Vertex const & v) const
    {
      auto const piF = graph.HeuristicCostEstimate(v, finalVertex);
      auto const piR = graph.HeuristicCostEstimate(v, startVertex);
      if (forward)
      {
        /// @todo careful: with this "return" here and below in the Backward case
        /// the heuristic becomes inconsistent but still seems to work.
        /// return HeuristicCostEstimate(v, finalVertex);
        return 0.5 * (piF - piR + m_piRT);
      }
      else
      {
        // return HeuristicCostEstimate(v, startVertex);
        return 0.5 * (piR - piF + m_piFS);
      }
    }

    void GetAdjacencyList(Vertex const & v, std::vector<Edge> & adj)
    {
      if (forward)
        graph.GetOutgoingEdgesList(v, adj);
      else
        graph.GetIngoingEdgesList(v, adj);
    }

    bool const forward;
    Vertex const & startVertex;
    Vertex const & finalVertex;
    Graph & graph;
    Weight const m_piRT;
    Weight const m_piFS;

    std::priority_queue<State, std::vector<State>, std::greater<State>> queue;
    std::map<Vertex, Weight> bestDistance;
    std::map<Vertex, Vertex> parent;
    Vertex bestVertex;

    Weight pS;
  };

  static void ReconstructPath(Vertex const & v, std::map<Vertex, Vertex> const & parent,
                              std::vector<Vertex> & path);
  static void ReconstructPathBidirectional(Vertex const & v, Vertex const & w,
                                           std::map<Vertex, Vertex> const & parentV,
                                           std::map<Vertex, Vertex> const & parentW,
                                           std::vector<Vertex> & path);
};

template <typename Graph>
constexpr typename Graph::Weight AStarAlgorithm<Graph>::kEpsilon;
template <typename Graph>
constexpr typename Graph::Weight AStarAlgorithm<Graph>::kInfiniteDistance;
template <typename Graph>
constexpr typename Graph::Weight AStarAlgorithm<Graph>::kZeroDistance;

template <typename Graph>
template <typename VisitVertex, typename AdjustEdgeWeight, typename FilterStates>
void AStarAlgorithm<Graph>::PropagateWave(Graph & graph, Vertex const & startVertex,
                                          VisitVertex && visitVertex,
                                          AdjustEdgeWeight && adjustEdgeWeight,
                                          FilterStates && filterStates,
                                          AStarAlgorithm<Graph>::Context & context) const
{
  context.Clear();

  std::priority_queue<State, std::vector<State>, std::greater<State>> queue;

  context.SetDistance(startVertex, kZeroDistance);
  queue.push(State(startVertex, kZeroDistance));

  std::vector<Edge> adj;

  while (!queue.empty())
  {
    State const stateV = queue.top();
    queue.pop();

    if (stateV.distance > context.GetDistance(stateV.vertex))
      continue;

    if (!visitVertex(stateV.vertex))
      return;

    graph.GetOutgoingEdgesList(stateV.vertex, adj);
    for (auto const & edge : adj)
    {
      State stateW(edge.GetTarget(), kZeroDistance);
      if (stateV.vertex == stateW.vertex)
        continue;

      auto const edgeWeight = adjustEdgeWeight(stateV.vertex, edge);
      auto const newReducedDist = stateV.distance + edgeWeight;

      if (newReducedDist >= context.GetDistance(stateW.vertex) - kEpsilon)
        continue;

      stateW.distance = newReducedDist;

      if (!filterStates(stateW))
        continue;

      context.SetDistance(stateW.vertex, newReducedDist);
      context.SetParent(stateW.vertex, stateV.vertex);
      queue.push(stateW);
    }
  }
}

template <typename Graph>
template <typename VisitVertex>
void AStarAlgorithm<Graph>::PropagateWave(Graph & graph, Vertex const & startVertex,
                                          VisitVertex && visitVertex,
                                          AStarAlgorithm<Graph>::Context & context) const
{
  auto const adjustEdgeWeight = [](Vertex const & /* vertex */, Edge const & edge) {
    return edge.GetWeight();
  };
  auto const filterStates = [](State const & /* state */) { return true; };
  PropagateWave(graph, startVertex, visitVertex, adjustEdgeWeight, filterStates, context);
}

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

template <typename Graph>
template <typename P>
typename AStarAlgorithm<Graph>::Result AStarAlgorithm<Graph>::FindPath(
    P & params, RoutingResult<Vertex, Weight> & result) const
{
  result.Clear();

  auto & graph = params.m_graph;
  auto const & finalVertex = params.m_finalVertex;
  auto const & startVertex = params.m_startVertex;

  Context context;
  PeriodicPollCancellable periodicCancellable(params.m_cancellable);
  Result resultCode = Result::NoPath;

  auto const heuristicDiff = [&](Vertex const & vertexFrom, Vertex const & vertexTo) {
    return graph.HeuristicCostEstimate(vertexFrom, finalVertex) -
           graph.HeuristicCostEstimate(vertexTo, finalVertex);
  };

  auto const fullToReducedLength = [&](Vertex const & vertexFrom, Vertex const & vertexTo,
                                       Weight const length) {
    return length - heuristicDiff(vertexFrom, vertexTo);
  };

  auto const reducedToFullLength = [&](Vertex const & vertexFrom, Vertex const & vertexTo,
                                       Weight const reducedLength) {
    return reducedLength + heuristicDiff(vertexFrom, vertexTo);
  };

  auto visitVertex = [&](Vertex const & vertex) {
    if (periodicCancellable.IsCancelled())
    {
      resultCode = Result::Cancelled;
      return false;
    }

    params.m_onVisitedVertexCallback(vertex, finalVertex);

    if (vertex == finalVertex)
    {
      resultCode = Result::OK;
      return false;
    }

    return true;
  };

  auto const adjustEdgeWeight = [&](Vertex const & vertexV, Edge const & edge) {
    auto const reducedWeight = fullToReducedLength(vertexV, edge.GetTarget(), edge.GetWeight());

    CHECK_GREATER_OR_EQUAL(reducedWeight, -kEpsilon, ("Invariant violated."));

    return std::max(reducedWeight, kZeroDistance);
  };

  auto const filterStates = [&](State const & state) {
    return params.m_checkLengthCallback(
        reducedToFullLength(startVertex, state.vertex, state.distance));
  };

  PropagateWave(graph, startVertex, visitVertex, adjustEdgeWeight, filterStates, context);

  if (resultCode == Result::OK)
  {
    context.ReconstructPath(finalVertex, result.m_path);
    result.m_distance =
        reducedToFullLength(startVertex, finalVertex, context.GetDistance(finalVertex));

    if (!params.m_checkLengthCallback(result.m_distance))
      resultCode = Result::NoPath;
  }

  return resultCode;
}

template <typename Graph>
template <typename P>
typename AStarAlgorithm<Graph>::Result AStarAlgorithm<Graph>::FindPathBidirectional(
    P & params, RoutingResult<Vertex, Weight> & result) const
{
  auto & graph = params.m_graph;
  auto const & finalVertex = params.m_finalVertex;
  auto const & startVertex = params.m_startVertex;

  BidirectionalStepContext forward(true /* forward */, startVertex, finalVertex, graph);
  BidirectionalStepContext backward(false /* forward */, startVertex, finalVertex, graph);

  bool foundAnyPath = false;
  auto bestPathReducedLength = kZeroDistance;
  auto bestPathRealLength = kZeroDistance;

  forward.bestDistance[startVertex] = kZeroDistance;
  forward.queue.push(State(startVertex, kZeroDistance));

  backward.bestDistance[finalVertex] = kZeroDistance;
  backward.queue.push(State(finalVertex, kZeroDistance));

  // To use the search code both for backward and forward directions
  // we keep the pointers to everything related to the search in the
  // 'current' and 'next' directions. Swapping these pointers indicates
  // changing the end we are searching from.
  BidirectionalStepContext * cur = &forward;
  BidirectionalStepContext * nxt = &backward;

  std::vector<Edge> adj;

  // It is not necessary to check emptiness for both queues here
  // because if we have not found a path by the time one of the
  // queues is exhausted, we never will.
  uint32_t steps = 0;
  PeriodicPollCancellable periodicCancellable(params.m_cancellable);

  while (!cur->queue.empty() && !nxt->queue.empty())
  {
    ++steps;

    if (periodicCancellable.IsCancelled())
      return Result::Cancelled;

    if (steps % kQueueSwitchPeriod == 0)
      std::swap(cur, nxt);

    if (foundAnyPath)
    {
      auto const curTop = cur->TopDistance();
      auto const nxtTop = nxt->TopDistance();

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
        if (!params.m_checkLengthCallback(bestPathRealLength))
          return Result::NoPath;

        ReconstructPathBidirectional(cur->bestVertex, nxt->bestVertex, cur->parent, nxt->parent,
                                     result.m_path);
        result.m_distance = bestPathRealLength;
        CHECK(!result.m_path.empty(), ());
        if (!cur->forward)
          reverse(result.m_path.begin(), result.m_path.end());
        return Result::OK;
      }
    }

    State const stateV = cur->queue.top();
    cur->queue.pop();

    if (stateV.distance > cur->bestDistance[stateV.vertex])
      continue;

    params.m_onVisitedVertexCallback(stateV.vertex,
                                     cur->forward ? cur->finalVertex : cur->startVertex);

    cur->GetAdjacencyList(stateV.vertex, adj);
    for (auto const & edge : adj)
    {
      State stateW(edge.GetTarget(), kZeroDistance);
      if (stateV.vertex == stateW.vertex)
        continue;

      auto const weight = edge.GetWeight();
      auto const pV = cur->ConsistentHeuristic(stateV.vertex);
      auto const pW = cur->ConsistentHeuristic(stateW.vertex);
      auto const reducedWeight = weight + pW - pV;

      CHECK_GREATER_OR_EQUAL(reducedWeight, -kEpsilon,
                             ("Invariant violated for:", "v =", stateV.vertex, "w =", stateW.vertex));

      auto const newReducedDist = stateV.distance + std::max(reducedWeight, kZeroDistance);

      auto const fullLength = weight + stateV.distance + cur->pS - pV;
      if (!params.m_checkLengthCallback(fullLength))
        continue;

      auto const itCur = cur->bestDistance.find(stateW.vertex);
      if (itCur != cur->bestDistance.end() && newReducedDist >= itCur->second - kEpsilon)
        continue;

      auto const itNxt = nxt->bestDistance.find(stateW.vertex);
      if (itNxt != nxt->bestDistance.end())
      {
        auto const distW = itNxt->second;
        // Reduced length that the path we've just found has in the original graph:
        // find the reduced length of the path's parts in the reduced forward and backward graphs.
        auto const curPathReducedLength = newReducedDist + distW;
        // No epsilon here: it is ok to overshoot slightly.
        if (!foundAnyPath || bestPathReducedLength > curPathReducedLength)
        {
          bestPathReducedLength = curPathReducedLength;

          bestPathRealLength = stateV.distance + weight + distW;
          bestPathRealLength += cur->pS - pV;
          bestPathRealLength += nxt->pS - nxt->ConsistentHeuristic(stateW.vertex);

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

template <typename Graph>
template <typename P>
typename AStarAlgorithm<Graph>::Result AStarAlgorithm<Graph>::AdjustRoute(
    P & params, RoutingResult<Vertex, Weight> & result) const
{
  CHECK(params.m_prevRoute, ());
  auto & graph = params.m_graph;
  auto const & startVertex = params.m_startVertex;
  auto const & prevRoute = *params.m_prevRoute;

  CHECK(!prevRoute.empty(), ());

  CHECK(params.m_checkLengthCallback != nullptr,
        ("CheckLengthCallback expected to be set to limit wave propagation."));

  result.Clear();

  bool wasCancelled = false;
  auto minDistance = kInfiniteDistance;
  Vertex returnVertex;

  std::map<Vertex, Weight> remainingDistances;
  auto remainingDistance = kZeroDistance;

  for (auto it = prevRoute.crbegin(); it != prevRoute.crend(); ++it)
  {
    remainingDistances[it->GetTarget()] = remainingDistance;
    remainingDistance += it->GetWeight();
  }

  Context context;
  PeriodicPollCancellable periodicCancellable(params.m_cancellable);

  auto visitVertex = [&](Vertex const & vertex) {

    if (periodicCancellable.IsCancelled())
    {
      wasCancelled = true;
      return false;
    }

    params.m_onVisitedVertexCallback(startVertex, vertex);

    auto it = remainingDistances.find(vertex);
    if (it != remainingDistances.cend())
    {
      auto const fullDistance = context.GetDistance(vertex) + it->second;
      if (fullDistance < minDistance)
      {
        minDistance = fullDistance;
        returnVertex = vertex;
      }
    }

    return true;
  };

  auto const adjustEdgeWeight = [](Vertex const & /* vertex */, Edge const & edge) {
    return edge.GetWeight();
  };

  auto const filterStates = [&](State const & state) {
    return params.m_checkLengthCallback(state.distance);
  };

  PropagateWave(graph, startVertex, visitVertex, adjustEdgeWeight, filterStates, context);
  if (wasCancelled)
    return Result::Cancelled;

  if (minDistance == kInfiniteDistance)
    return Result::NoPath;

  context.ReconstructPath(returnVertex, result.m_path);

  // Append remaining route.
  bool found = false;
  for (size_t i = 0; i < prevRoute.size(); ++i)
  {
    if (prevRoute[i].GetTarget() == returnVertex)
    {
      for (size_t j = i + 1; j < prevRoute.size(); ++j)
        result.m_path.push_back(prevRoute[j].GetTarget());

      found = true;
      break;
    }
  }

  CHECK(found, ("Can't find", returnVertex, ", prev:", prevRoute.size(),
                ", adjust:", result.m_path.size()));

  auto const & it = remainingDistances.find(returnVertex);
  CHECK(it != remainingDistances.end(), ());
  result.m_distance = context.GetDistance(returnVertex) + it->second;
  return Result::OK;
}

// static
template <typename Graph>
void AStarAlgorithm<Graph>::ReconstructPath(Vertex const & v,
                                            std::map<Vertex, Vertex> const & parent,
                                            std::vector<Vertex> & path)
{
  path.clear();
  Vertex cur = v;
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
template <typename Graph>
void AStarAlgorithm<Graph>::ReconstructPathBidirectional(Vertex const & v, Vertex const & w,
                                                         std::map<Vertex, Vertex> const & parentV,
                                                         std::map<Vertex, Vertex> const & parentW,
                                                         std::vector<Vertex> & path)
{
  std::vector<Vertex> pathV;
  ReconstructPath(v, parentV, pathV);
  std::vector<Vertex> pathW;
  ReconstructPath(w, parentW, pathW);
  path.clear();
  path.reserve(pathV.size() + pathW.size());
  path.insert(path.end(), pathV.begin(), pathV.end());
  path.insert(path.end(), pathW.rbegin(), pathW.rend());
}

template <typename Graph>
void AStarAlgorithm<Graph>::Context::ReconstructPath(Vertex const & v,
                                                     std::vector<Vertex> & path) const
{
  AStarAlgorithm<Graph>::ReconstructPath(v, m_parents, path);
}
}  // namespace routing
