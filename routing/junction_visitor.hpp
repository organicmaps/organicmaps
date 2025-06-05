#pragma once

#include "routing/base/astar_progress.hpp"

#include "routing/router_delegate.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>

namespace routing
{
double constexpr kProgressInterval = 0.5;

#ifdef DEBUG
inline void DebugRoutingState(...) {}

class JointSegment;
class RouteWeight;
class Segment;
void DebugRoutingState(Segment const & vertex, std::optional<Segment> const & parent, RouteWeight const & heuristic,
                       RouteWeight const & distance);
void DebugRoutingState(JointSegment const & vertex, std::optional<JointSegment> const & parent,
                       RouteWeight const & heuristic, RouteWeight const & distance);
#endif

template <typename Graph>
class JunctionVisitor
{
public:
  using Vertex = typename Graph::Vertex;

  JunctionVisitor(Graph & graph, RouterDelegate const & delegate, uint32_t visitPeriod,
                  std::shared_ptr<AStarProgress> const & progress = nullptr)
    : m_graph(graph)
    , m_delegate(delegate)
    , m_visitPeriod(visitPeriod)
    , m_progress(progress)
  {
    if (progress)
      m_lastProgressPercent = progress->GetLastPercent();
  }

  /// @param[in]  p   { Current state, Step context } pair.
  /// @param[in]  to  End vertex (final for forward and start for backward waves).
  template <class StateContextPair>
  void operator()(StateContextPair const & p, Vertex const & to)
  {
    auto const & state = p.first;
#ifdef DEBUG
    // For Debug purpose.
    DebugRoutingState(state.vertex, p.second->GetParent(state.vertex), state.heuristic, state.distance);
#endif
    this->operator()(state.vertex, to);
  }

  /// @param[in]  from  Current processing vertex.
  /// @param[in]  to    End vertex (final for forward and start for backward waves).
  void operator()(Vertex const & from, Vertex const & to)
  {
    ++m_visitCounter;
    if (m_visitCounter % m_visitPeriod != 0)
      return;

    auto const & pointFrom = m_graph.GetPoint(from, true /* front */);
    m_delegate.OnPointCheck(pointFrom);

    auto progress = m_progress.lock();
    if (!progress)
      return;

    auto const & pointTo = m_graph.GetPoint(to, true /* front */);
    auto const currentPercent = progress->UpdateProgress(pointFrom, pointTo);
    if (currentPercent - m_lastProgressPercent > kProgressInterval)
    {
      m_lastProgressPercent = currentPercent;
      m_delegate.OnProgress(currentPercent);
    }
  }

private:
  Graph & m_graph;
  RouterDelegate const & m_delegate;
  uint32_t m_visitCounter = 0;
  uint32_t m_visitPeriod;
  std::weak_ptr<AStarProgress> m_progress;
  double m_lastProgressPercent = 0.0;
};
}  // namespace routing
