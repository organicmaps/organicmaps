#pragma once

#include "indexer/terrain/terrain_reader.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <vector>

namespace terrain
{
// One traced isoline: the level altitude and the polyline in mercator.
// A closed ring stores the first point again as the last one.
struct Isoline
{
  Altitude m_altitude = 0;
  std::vector<m2::PointD> m_points;
  bool m_closed = false;
};

// Selects the isolines step in meters for the altitudes range: the smallest standard step
// keeping the levels count reasonable (10 m by default, coarser for high ranges).
Altitude SelectIsolinesStep(Altitude minAltitude, Altitude maxAltitude);

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

  // Traces the isolines of all the altitude levels multiple of step (meters) over the
  // features intersecting the mercator rect at the given geometry scale index.
  // Chains are maximal: they close into rings or end on the collected mesh boundary,
  // so the caller should inflate the rect if the isolines must cover it entirely.
  void Trace(m2::RectD const & rect, size_t geomIndex, Altitude step, IsolineFn const & fn) const;

private:
  std::vector<Reader const *> m_readers;
};
}  // namespace terrain
