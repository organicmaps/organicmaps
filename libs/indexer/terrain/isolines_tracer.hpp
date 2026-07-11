#pragma once

#include "indexer/terrain/terrain_reader.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <vector>

namespace terrain
{
// One traced isoline: the level altitude (in the units of the trace, see Trace) and the
// polyline in mercator. A closed ring stores the first point again as the last one.
// int32_t, not the storage Altitude: the feet values overflow int16_t for the deep bathymetry.
struct Isoline
{
  int32_t m_altitude = 0;
  std::vector<m2::PointD> m_points;
  bool m_closed = false;
};

// Extracts isolines from TWM terrain meshes by marching triangles:
// - a single pass per query serves all the levels: triangles are binned by the levels
//   they cross, so flat regions cost nothing;
// - a vertex exactly on a level counts as the higher side (the half-open classification),
//   so degenerate segments never appear and flat triangles are routed around - the same
//   convention as the raster marching squares corner shift in topography_generator;
// - a crossed triangle has exactly one high->low directed edge (the exit) and one
//   low->high edge (the entry), so chains are walked deterministically through the
//   adjacency and the higher ground always stays on the RIGHT of the traversal
//   direction, matching the marching squares orientation;
// - the adjacency is restored by hashing quantized directed edges, and the crossing
//   points are interpolated on canonically ordered edges: both incident triangles
//   produce bit-identical points, so chains continue seamlessly across triangles,
//   features and neighbor TWM blocks.
class IsolinesTracer
{
public:
  // All the readers must share the coordinate bits and geometry scales configuration.
  explicit IsolinesTracer(std::vector<Reader const *> const & readers);

  using IsolineFn = std::function<void(Isoline &&)>;

  // Traces the isolines of all the altitude levels multiple of step over the features
  // intersecting the mercator rect at the given geometry scale index. The stored meter
  // altitudes are converted to feet for the imperial units, so the step and the result
  // levels are round values in the display units.
  // Chains are maximal: they close into rings or end on the collected mesh boundary,
  // so the caller should inflate the rect if the isolines must cover it entirely.
  void Trace(m2::RectD const & rect, size_t geomIndex, int32_t step, measurement_utils::Units units,
             IsolineFn const & fn) const;

private:
  std::vector<Reader const *> m_readers;
};
}  // namespace terrain
