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
  // A conforming triangulation of the water union, including every known land hole.
  // Overlapping polygons must be unioned before triangulation.
  std::vector<std::array<m2::PointD, 3>> m_triangles;
  // Missing or grade-separated topology remains unknown here, never dry land.
  std::vector<m2::RectD> m_unknownAreas;
  // False for missing maps, unresolved topology or unknown local coverage.
  bool m_complete = false;
  // Display geometry lacks source node identities; transfers use stored line endpoints only.
  bool m_hasSourceTopology = true;
  std::shared_ptr<m4::Tree<size_t>> m_triangleIndex;
  // Complete local coverage, if extraction is bounded. Artificial boundary cuts
  // never prove disconnection. A connected path is accepted only with a bound
  // excluding a shorter excursion outside this rectangle.
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
  double m_waterDistance = 0.0;
  double m_landDistance = 0.0;
};

using CancelledFn = std::function<bool()>;
// Work-based estimate in [0, 1]; 1 is reported only for a successful path.
using ProgressFn = std::function<void(double)>;

ResultCode FindPath(Geometry const & geometry, m2::PointD const & start, m2::PointD const & finish, bool projectStart,
                    CancelledFn const & cancelled, Path & path, ProgressFn const & progress = {});
}  // namespace routing::water
