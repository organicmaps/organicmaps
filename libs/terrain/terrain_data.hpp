#pragma once

#include "terrain/raster_patch.hpp"

#include <optional>

namespace terrain
{
struct TerrainData
{
  TileMesh m_mesh;
  std::vector<RasterPatch> m_rasterPatches;

  bool IsEmpty() const { return m_mesh.IsEmpty() && m_rasterPatches.empty(); }

  // Interpolates the rendered triangles in meters; empty outside the collected surface. A linear scan.
  std::optional<double> GetElevation(m2::PointD const & point) const;
};
}  // namespace terrain
