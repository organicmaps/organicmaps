#pragma once

#include "base/scope_guard.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <vector>

namespace routing
{
template <typename Graph>
class BFS
{
public:
  using Vertex = typename Graph::Vertex;
  using Edge = typename Graph::Edge;
  using Weight = typename Graph::Weight;

  struct State
  {
    State(Vertex const & v, Vertex const & p) : m_vertex(v), m_parent(p) {}

    Vertex m_vertex;
    Vertex m_parent;
  };

  explicit BFS(Graph & graph) : m_graph(graph) {}

  void Run(Vertex const & start, bool isOutgoing, std::function<bool(State const &)> && onVisitCallback);

  std::vector<Vertex> ReconstructPath(Vertex from, bool reverse);

private:
  Graph & m_graph;
  std::map<Vertex, Vertex> m_parents;
};

template <typename Graph>
void BFS<Graph>::Run(Vertex const & start, bool isOutgoing, std::function<bool(State const &)> && onVisitCallback)
{
  m_parents.clear();

  m_parents.emplace(start, start);
  SCOPE_GUARD(removeStart, [&]() { m_parents.erase(start); });

  std::queue<Vertex> queue;
  queue.emplace(start);

  typename Graph::EdgeListT edges;
  while (!queue.empty())
  {
    Vertex const current = queue.front();
    queue.pop();

    m_graph.GetEdgesList(current, isOutgoing, edges);

    for (auto const & edge : edges)
    {
      Vertex const & child = edge.GetTarget();
      if (m_parents.count(child) != 0)
        continue;

      State const state(child, current);
      if (!onVisitCallback(state))
        continue;

      m_parents.emplace(child, current);
      queue.emplace(child);
    }
  }
}

template <typename Graph>
auto BFS<Graph>::ReconstructPath(Vertex from, bool reverse) -> std::vector<Vertex>
{
  std::vector<Vertex> result;
  auto it = m_parents.find(from);
  while (it != m_parents.end())
  {
    result.emplace_back(from);
    from = it->second;
    it = m_parents.find(from);
  }

  // Here stored path in inverse order (from end to begin).
  result.emplace_back(from);

  if (!reverse)
    std::reverse(result.begin(), result.end());

  return result;
}
}  // namespace routing
