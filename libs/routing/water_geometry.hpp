#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace routing::water
{
// Shared extraction/search ceilings: reject oversized geometry consistently before building the graph.
inline constexpr size_t kMaxGraphVertices = 524288;
inline constexpr size_t kMaxTriangles = 262144;
inline constexpr size_t kMaxWaterFeatures = 65536;

// IDs represent source nodes or exact same-layer stored endpoints in ordinary MWMs.
// Coordinate proximity and artificial clipping cuts never establish identity.
struct Vertex
{
  m2::PointD m_point;
  uint64_t m_nodeId = 0;
  bool m_mayTransfer = true;
};

// A balanced bounding-volume hierarchy prunes both axes at every level, including
// the many aligned triangles along river banks. Leaves retain original triangle IDs.
class TriangleIndex
{
public:
  explicit TriangleIndex(std::vector<std::array<m2::PointD, 3>> const & triangles);

  template <typename Fn>
  void ForEachInRect(m2::RectD const & bounds, Fn && fn) const
  {
    if (!m_nodes.empty())
      Visit(0, bounds, fn);
  }

private:
  struct Entry
  {
    m2::RectD m_bounds;
    size_t m_id;
  };
  struct Node
  {
    m2::RectD m_bounds;
    size_t m_begin;
    size_t m_end;
    size_t m_left = 0;
    size_t m_right = 0;
  };
  size_t Build(size_t begin, size_t end);

  template <typename Fn>
  void Visit(size_t index, m2::RectD const & bounds, Fn const & fn) const
  {
    auto const & node = m_nodes[index];
    if (!node.m_bounds.IsIntersect(bounds))
      return;
    if (node.m_left != 0)
    {
      Visit(node.m_left, bounds, fn);
      Visit(node.m_right, bounds, fn);
      return;
    }
    for (size_t i = node.m_begin; i < node.m_end; ++i)
      if (m_entries[i].m_bounds.IsIntersect(bounds))
        fn(m_entries[i].m_id);
  }

  std::vector<Entry> m_entries;
  std::vector<Node> m_nodes;
};

struct Geometry
{
  // Surface waterways only. Grade-separated overlaps require explicit transitions upstream.
  std::vector<std::vector<Vertex>> m_lines;
  // Triangulation of the water union, including every known land hole.
  // Overlapping polygons must be unioned before triangulation.
  std::vector<std::array<m2::PointD, 3>> m_triangles;
  // Missing or grade-separated topology remains unknown here, never dry land.
  std::vector<m2::RectD> m_unknownAreas;
  // Coarse coverage gaps are only a filter; intersecting segments need certification.
  std::vector<m2::RectD> m_coverageGaps;
  std::shared_ptr<m4::Tree<size_t>> m_coverageIndex;
  std::function<bool(m2::PointD const &, m2::PointD const &)> m_isCovered;
  // Mapped underground water may be bypassed on foot between stored river endpoints.
  std::vector<m2::RectD> m_portageAreas;
  // False for missing maps, unresolved topology or unknown local coverage.
  bool m_complete = false;
  bool m_resourceLimited = false;
  // Selectively loaded geometry cannot establish global shortest-path optimality.
  bool m_selective = false;
  // Display geometry lacks source node identities; transfers use stored line endpoints only.
  bool m_hasSourceTopology = true;
  std::shared_ptr<TriangleIndex> m_triangleIndex;
  // Extraction window. Unknown areas inside it remain excluded from all edges.
  // Artificial boundary cuts never prove disconnection; uncertain optimality
  // is reported separately from the validity of a known path.
  std::optional<m2::RectD> m_bounds;
};

enum class ResultCode
{
  Success,
  OutsideWater,
  Unavailable,
  Cancelled
};

struct Path
{
  std::array<bool, 4> m_frontiers{};
  bool m_bothEndsReachBoundary = false;
  std::vector<m2::PointD> m_points;
  // One classification per edge, including any explicitly permitted land gap.
  std::vector<bool> m_landGaps;
  bool m_shortestConfirmed = true;
  double m_waterDistance = 0.0;
  double m_landDistance = 0.0;
};

using CancelledFn = std::function<bool()>;
// Work-based estimate in [0, 1]; 1 is reported only for a successful path.
using ProgressFn = std::function<void(double)>;
// Called lazily for a proposed endpoint walk; false excludes that directed link.
using WalkAllowedFn = std::function<bool(m2::PointD const &, m2::PointD const &)>;

// Certify a straight surface-water segment; used only to guide selective loading.
bool CoversSurfaceSegment(Geometry const & geometry, m2::PointD const & from, m2::PointD const & to,
                          CancelledFn const & cancelled);

// A nearby entry point for a separately calculated pedestrian approach.
std::optional<m2::PointD> FindApproachPoint(Geometry const & geometry, m2::PointD const & start,
                                            CancelledFn const & cancelled);

// Unconfirmed transfers are a last resort after a wider water search; they still
// require eligible endpoints and cannot cross unknown geometry.
ResultCode FindPath(Geometry const & geometry, m2::PointD const & start, m2::PointD const & finish, bool projectStart,
                    CancelledFn const & cancelled, Path & path, ProgressFn const & progress = {},
                    bool allowUnconfirmedTransfer = false, WalkAllowedFn const & walkAllowed = {});
}  // namespace routing::water
