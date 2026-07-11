#pragma once

#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "geometry/rect2d.hpp"

#include <functional>
#include <string>

namespace terrain
{
// The provider of the dynamic isolines over the downloaded .twm terrain files:
// scans the directory into the TwmSet registry and serves the queries from the drape tile reading threads.
// The set hands the opened readers off exclusively (concurrent queries get own values), so the FileReader caches need
// no sharing; blocks detected corrupt are condemned and never retried.
class TerrainProvider
{
public:
  // dir is the directory with the .twm files.
  explicit TerrainProvider(std::string const & dir) : m_dir(dir) {}

  // Rescans the directory: registers the new files, deregisters the vanished ones
  // (delayed for the blocks locked by the running queries).
  void Rescan();
  void Clear();

  // Returns true if any registered terrain block intersects the mercator rect.
  // Cheap registry lookup, safe for the UI thread.
  bool HasTerrain(m2::RectD const & rect) const { return m_set.HasBlocks(rect); }

  using IsolineFn = std::function<void(Isoline &&)>;

  // Traces the isolines covering the mercator rect with the step and the geometry scale
  // selected for the draw zoom. Called from the drape tile reading threads.
  void ForEachIsoline(m2::RectD const & rect, int zoom, IsolineFn const & fn) const;

  using TrianglesFn = std::function<void(Triangles const &)>;

  // Reads the raw terrain triangles of the features intersecting the mercator rect at the
  // geometry scale selected for the draw zoom. Called from the drape tile reading threads.
  void ForEachTriangles(m2::RectD const & rect, int zoom, TrianglesFn const & fn) const;

private:
  std::string m_dir;
  // Mutable: the const queries lock the readers and condemn the corrupt blocks.
  mutable TwmSet m_set;
};
}  // namespace terrain
