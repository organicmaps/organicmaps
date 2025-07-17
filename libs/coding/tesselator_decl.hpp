#pragma once

#include "base/assert.hpp"

#include <cstdint>

namespace tesselator
{
// Edge of graph, built from triangles list.
struct Edge
{
  int m_p[2];        // indexes of connected triangles (0 -> 1)
  uint64_t m_delta;  // delta of 1 - triangle from 0 - triangle

  // intersected rib of 0 - triangle:
  // - -1 - uninitialized or root edge
  // -  0 - this edge intersects 1-2 rib;
  // -  1 - this edge intersects 2-0 rib;
  int8_t m_side;

  Edge(int from, int to, uint64_t delta, char side) : m_delta(delta), m_side(side)
  {
    m_p[0] = from;
    m_p[1] = to;
  }
};
}  // namespace tesselator
