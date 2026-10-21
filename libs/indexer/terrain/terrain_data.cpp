#include "indexer/terrain/terrain_data.hpp"

#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"
#include "geometry/triangle2d.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

namespace terrain
{
std::optional<double> TerrainData::GetElevation(m2::PointD const & point) const
{
  for (auto const & patch : m_rasterPatches)
  {
    if (!patch.GetRect().IsPointInside(point))
      continue;

    // Pick a cell without quantizing the query: the returned altitude follows the
    // same continuous surface as the rendered mesh, including its outer edges.
    auto const cell = [&](std::vector<uint32_t> const & axis, double coordinate, double minC, double maxC)
    {
      auto const it = std::lower_bound(axis.begin() + 1, axis.end(), coordinate, [&](uint32_t value, double c)
      { return Uint32ToDouble(value, minC, maxC, patch.m_coordBits) < c; });
      return static_cast<size_t>(it - axis.begin() - 1);
    };
    size_t const x = cell(patch.m_columns, point.x, mercator::Bounds::kMinX, mercator::Bounds::kMaxX);
    size_t const y = cell(patch.m_rows, point.y, mercator::Bounds::kMinY, mercator::Bounds::kMaxY);
    size_t const sw = y * patch.m_columns.size() + x;
    size_t const ne = sw + patch.m_columns.size() + 1;
    auto const a = patch.Point(static_cast<uint32_t>(sw)), b = patch.Point(static_cast<uint32_t>(ne));
    double const u = (point.x - a.x) / (b.x - a.x), v = (point.y - a.y) / (b.y - a.y);
    auto const & h = patch.m_heights;
    // RasterPatch::Triangle uses a fixed SW-NE diagonal, not bilinear interpolation.
    if (u >= v)
      return (1.0 - u) * h[sw] + (u - v) * h[sw + 1] + v * h[ne];
    return (1.0 - v) * h[sw] + u * h[ne] + (v - u) * h[ne - 1];
  }

  auto const & points = m_mesh.GetPoints();
  auto const & triangles = m_mesh.GetTriangles();
  auto const & heights = m_mesh.GetAltitudes();
  for (size_t i = 0; i < triangles.size(); i += 3)
  {
    auto const a = triangles[i], b = triangles[i + 1], c = triangles[i + 2];
    if (!m2::IsPointInsideTriangle(point, points[a], points[b], points[c]))
      continue;
    double const area = m2::robust::OrientedS(points[a], points[b], points[c]);
    ASSERT_NOT_EQUAL(area, 0.0, ());
    double const wa = m2::robust::OrientedS(point, points[b], points[c]) / area;
    double const wb = m2::robust::OrientedS(points[a], point, points[c]) / area;
    return wa * heights[a] + wb * heights[b] + (1.0 - wa - wb) * heights[c];
  }
  return std::nullopt;
}
}  // namespace terrain
