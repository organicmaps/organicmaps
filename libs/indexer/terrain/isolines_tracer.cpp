#include "indexer/terrain/isolines_tracer.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace terrain
{
namespace
{
uint32_t constexpr kNone = std::numeric_limits<uint32_t>::max();

// The traced view over the deduplicated tile mesh: the altitudes are in the display
// units, the adjacency map sews the triangles across features and blocks.
struct TracedMesh
{
  std::vector<m2::PointD> const & m_points;
  std::vector<int32_t> const & m_altitudes;
  std::vector<uint32_t> const & m_triangles;
  // Directed edge (u << 32 | v) -> the owning triangle.
  std::unordered_map<uint64_t, uint32_t> m_edgeToTri;

  static uint64_t EdgeKey(uint32_t u, uint32_t v) { return (static_cast<uint64_t>(u) << 32) | v; }

  uint32_t Vertex(uint32_t tri, size_t i) const { return m_triangles[tri * 3 + i]; }

  uint32_t Neighbor(uint32_t u, uint32_t v) const
  {
    auto const it = m_edgeToTri.find(EdgeKey(v, u));
    return it == m_edgeToTri.end() ? kNone : it->second;
  }

  // The crossing point of the level h on the (u, v) edge. The vertices are reordered
  // canonically, so both incident triangles produce a bit-identical point.
  m2::PointD Crossing(uint32_t u, uint32_t v, int32_t h) const
  {
    if (u > v)
      std::swap(u, v);
    double const au = m_altitudes[u];
    double const av = m_altitudes[v];
    ASSERT(au != av, (h));
    double const t = (h - au) / (av - au);
    return m_points[u] + (m_points[v] - m_points[u]) * t;
  }
};

struct CrossedEdges
{
  // The entry (low -> high) and the exit (high -> low) directed edges of a crossed triangle.
  uint32_t m_entryU = kNone, m_entryV = kNone;
  uint32_t m_exitU = kNone, m_exitV = kNone;
};

// A vertex exactly on the level counts as the higher side (half-open classification).
bool IsHigh(int32_t alt, int32_t h)
{
  return alt >= h;
}

CrossedEdges GetCrossedEdges(TracedMesh const & mesh, uint32_t tri, int32_t h)
{
  CrossedEdges edges;
  for (size_t i = 0; i < 3; ++i)
  {
    uint32_t const u = mesh.Vertex(tri, i);
    uint32_t const v = mesh.Vertex(tri, (i + 1) % 3);
    bool const highU = IsHigh(mesh.m_altitudes[u], h);
    bool const highV = IsHigh(mesh.m_altitudes[v], h);
    if (highU == highV)
      continue;
    if (highU)
    {
      edges.m_exitU = u;
      edges.m_exitV = v;
    }
    else
    {
      edges.m_entryU = u;
      edges.m_entryV = v;
    }
  }
  ASSERT(edges.m_entryU != kNone && edges.m_exitU != kNone, (tri, h));
  return edges;
}
}  // namespace

void TraceIsolines(TileMesh const & tileMesh, int32_t step, measurement_utils::Units units, IsolineFn const & fn)
{
  ASSERT_GREATER(step, 0, ());
  size_t const trianglesCount = tileMesh.GetTrianglesCount();
  if (trianglesCount == 0)
    return;

  // The display units altitudes: the rounded feet conversion is deterministic per
  // vertex, so the shared vertices still agree across features and blocks and the
  // chains stay seamless.
  std::vector<int32_t> feet;
  if (units == measurement_utils::Units::Imperial)
  {
    feet.reserve(tileMesh.GetAltitudes().size());
    for (int32_t const altitude : tileMesh.GetAltitudes())
      feet.push_back(static_cast<int32_t>(std::lround(measurement_utils::MetersToFeet(altitude))));
  }

  TracedMesh mesh{tileMesh.GetPoints(), feet.empty() ? tileMesh.GetAltitudes() : feet, tileMesh.GetTriangles(), {}};

  mesh.m_edgeToTri.reserve(trianglesCount * 3);
  for (uint32_t t = 0; t < trianglesCount; ++t)
    for (size_t i = 0; i < 3; ++i)
      CHECK(mesh.m_edgeToTri.emplace(TracedMesh::EdgeKey(mesh.Vertex(t, i), mesh.Vertex(t, (i + 1) % 3)), t).second,
            ());

  // Bin the triangles by the levels they cross: the level h crosses a triangle iff
  // min < h <= max over its vertex altitudes (the half-open classification).
  int32_t minLevel = std::numeric_limits<int32_t>::max();
  int32_t maxLevel = std::numeric_limits<int32_t>::min();
  std::vector<std::pair<int32_t, int32_t>> levelRanges(trianglesCount);
  for (uint32_t t = 0; t < trianglesCount; ++t)
  {
    auto const [minAlt, maxAlt] = std::minmax({mesh.m_altitudes[mesh.Vertex(t, 0)], mesh.m_altitudes[mesh.Vertex(t, 1)],
                                               mesh.m_altitudes[mesh.Vertex(t, 2)]});
    int32_t const lo = math::FloorDiv(minAlt, step) + 1;
    int32_t const hi = math::FloorDiv(maxAlt, step);

    // A triangle crossing this many levels is physically impossible for real terrain:
    // don't let a crafted altitudes range blow up the level bins below.
    CHECK_LESS(hi - lo, 1024, (minAlt, maxAlt, step));

    levelRanges[t] = {lo, hi};
    if (lo <= hi)
    {
      minLevel = std::min(minLevel, lo);
      maxLevel = std::max(maxLevel, hi);
    }
  }
  if (minLevel > maxLevel)
    return;

  std::vector<std::vector<uint32_t>> bins(maxLevel - minLevel + 1);
  for (uint32_t t = 0; t < trianglesCount; ++t)
    for (int32_t l = levelRanges[t].first; l <= levelRanges[t].second; ++l)
      bins[l - minLevel].push_back(t);

  // Trace the chains per level: walk forward through the exit edges, the higher ground
  // stays on the right by construction.
  std::vector<uint32_t> visited(trianglesCount, 0);
  for (size_t bin = 0; bin < bins.size(); ++bin)
  {
    int32_t const h = (minLevel + static_cast<int32_t>(bin)) * step;
    uint32_t const epoch = static_cast<uint32_t>(bin) + 1;
    for (uint32_t const seed : bins[bin])
    {
      if (visited[seed] == epoch)
        continue;

      // Walk backward through the entry edges to find the chain start or detect a ring.
      uint32_t start = seed;
      bool closed = false;
      for (size_t guard = 0; guard <= trianglesCount; ++guard)
      {
        auto const edges = GetCrossedEdges(mesh, start, h);
        uint32_t const prev = mesh.Neighbor(edges.m_entryU, edges.m_entryV);
        if (prev == kNone)
          break;
        if (prev == seed)
        {
          closed = true;
          start = seed;
          break;
        }
        start = prev;
        CHECK_LESS(guard, trianglesCount, ("The backward walk has not terminated", h));
      }

      Isoline isoline;
      isoline.m_altitude = h;
      isoline.m_closed = closed;
      auto & points = isoline.m_points;
      auto const append = [&points](m2::PointD const & pt)
      {
        // Chains passing exactly through a vertex produce repeated crossing points.
        if (points.empty() || points.back() != pt)
          points.push_back(pt);
      };

      uint32_t cur = start;
      for (size_t guard = 0; guard <= trianglesCount; ++guard)
      {
        auto const edges = GetCrossedEdges(mesh, cur, h);
        if (points.empty() && !closed)
          append(mesh.Crossing(edges.m_entryU, edges.m_entryV, h));
        append(mesh.Crossing(edges.m_exitU, edges.m_exitV, h));
        visited[cur] = epoch;

        uint32_t const next = mesh.Neighbor(edges.m_exitU, edges.m_exitV);
        if (next == kNone)
          break;
        if (next == start && closed)
        {
          append(points.front());
          break;
        }
        cur = next;
        CHECK_LESS(guard, trianglesCount, ("The forward walk has not terminated", h));
      }

      if (points.size() < (closed ? 4u : 2u))
        continue;  // Degenerate: the level touches the mesh at a single vertex.
      fn(std::move(isoline));
    }
  }
}
}  // namespace terrain
