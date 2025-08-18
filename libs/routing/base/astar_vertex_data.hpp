#pragma once

namespace routing::astar
{
template <typename Vertex, typename Weight>
struct VertexData
{
  static VertexData Zero() { return VertexData(Vertex(), Weight()); }

  VertexData(Vertex const & vertex, Weight const & realDistance) : m_vertex(vertex), m_realDistance(realDistance) {}

  Vertex m_vertex;
  Weight m_realDistance;
};
}  // namespace routing::astar
