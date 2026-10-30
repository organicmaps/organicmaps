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

// Node identities come from the source waterway topology, not coordinate proximity.
struct Vertex
{
  m2::PointD m_point;
  uint64_t m_nodeId = 0;
  bool m_mayTransfer = true;
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
  // Mapped underground water may be bypassed on foot between stored river endpoints.
  std::vector<m2::RectD> m_portageAreas;
  // False for missing maps, unresolved topology or unknown local coverage.
  bool m_complete = false;
  // Display geometry lacks source node identities; transfers use stored line endpoints only.
  bool m_hasSourceTopology = true;
  std::shared_ptr<m4::Tree<size_t>> m_triangleIndex;
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

// A nearby entry point for a separately calculated pedestrian approach.
std::optional<m2::PointD> FindApproachPoint(Geometry const & geometry, m2::PointD const & start,
                                            CancelledFn const & cancelled);

// Unconfirmed transfers are a last resort after a wider water search; they still
// require eligible endpoints and cannot cross unknown geometry.
ResultCode FindPath(Geometry const & geometry, m2::PointD const & start, m2::PointD const & finish, bool projectStart,
                    CancelledFn const & cancelled, Path & path, ProgressFn const & progress = {},
                    bool allowUnconfirmedTransfer = false, WalkAllowedFn const & walkAllowed = {});
}  // namespace routing::water
