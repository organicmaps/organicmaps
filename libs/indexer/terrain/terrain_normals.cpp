#include "indexer/terrain/terrain_normals.hpp"

#include "3party/skarupke/flat_hash_map.hpp"

#include <algorithm>
#include <utility>

namespace terrain
{
namespace
{
TerrainNormal Face(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c, int32_t za, int32_t zb, int32_t zc,
                   double zScale)
{
  auto const ab = b - a, ac = c - a;
  double const dzb = (zb - za) * zScale, dzc = (zc - za) * zScale;
  return {ab.y * dzc - dzb * ac.y, dzb * ac.x - ab.x * dzc, ab.x * ac.y - ab.y * ac.x};
}

// The cells of the axis nodes over [minimum, maximum], widened to the whole vertex fans of the nodes
// of the cells that only touch the range.
std::pair<uint32_t, uint32_t> GetCellsRange(std::vector<double> const & axis, double minimum, double maximum)
{
  int64_t constexpr kFanMargin = 2;
  int64_t const lastCell = static_cast<int64_t>(axis.size()) - 2;
  int64_t const first = std::upper_bound(axis.begin(), axis.end(), minimum) - axis.begin() - 1 - kFanMargin;
  int64_t const last = std::lower_bound(axis.begin(), axis.end(), maximum) - axis.begin() - 1 + kFanMargin;
  return {static_cast<uint32_t>(std::clamp<int64_t>(first, 0, lastCell)),
          static_cast<uint32_t>(std::clamp<int64_t>(last, 0, lastCell))};
}
}  // namespace

TerrainNormals CalculateTerrainNormals(TerrainData const & data, double zScale,
                                       base::SharedCancellable const & isCancellable, m2::RectD const & drawRect)
{
  TerrainNormals result;
  auto const & mesh = data.m_mesh;
  result.m_tin.resize(mesh.GetPoints().size());
  auto const & triangles = mesh.GetTriangles();
  for (size_t t = 0; t < triangles.size(); t += 3)
  {
    if ((t % 3072 == 0) && isCancellable.IsCancelled())
      return {};
    uint32_t const a = triangles[t], b = triangles[t + 1], c = triangles[t + 2];
    auto const n = Face(mesh.GetPoints()[a], mesh.GetPoints()[b], mesh.GetPoints()[c], mesh.GetAltitudes()[a],
                        mesh.GetAltitudes()[b], mesh.GetAltitudes()[c], zScale);
    for (auto const v : {a, b, c})
      result.m_tin[v] += n;
  }
  if (data.m_rasterPatches.empty())
    return result;
  ska::flat_hash_map<uint64_t, TerrainNormal, impl::PointKeyHash> interface;
  for (auto const & patch : data.m_rasterPatches)
  {
    if (isCancellable.IsCancelled())
      return {};
    result.m_raster.emplace_back(patch.m_heights.size());
    auto & normals = result.m_raster.back();
    // The per-node coordinate conversions would cost several times more than the faces.
    std::vector<double> xs, ys;
    xs.reserve(patch.m_columns.size());
    ys.reserve(patch.m_rows.size());
    for (auto const column : patch.m_columns)
      xs.push_back(PointUToPointD({column, 0}, kTerrainCoordBits).x);
    for (auto const row : patch.m_rows)
      ys.push_back(PointUToPointD({0, row}, kTerrainCoordBits).y);
    uint32_t const stride = patch.CellsX() + 1;
    auto const addFace =
        [&](std::array<uint32_t, 3> const & v, m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
    {
      auto const & heights = patch.m_heights;
      auto const n = Face(a, b, c, heights[v[0]], heights[v[1]], heights[v[2]], zScale);
      for (auto const node : v)
        normals[node] += n;
    };
    // A patch edge always counts: its nodes join the neighbor patches and the TIN vertices,
    // which can be drawn by a triangle reaching far beyond the rect.
    auto const [firstX, lastX] = GetCellsRange(xs, drawRect.minX(), drawRect.maxX());
    auto const [firstY, lastY] = GetCellsRange(ys, drawRect.minY(), drawRect.maxY());
    for (uint32_t y = 0; y < patch.CellsY(); ++y)
    {
      bool const edgeRow = y == 0 || y + 1 == patch.CellsY();
      bool const drawnRow = firstY <= y && y <= lastY;
      for (uint32_t x = 0; x < patch.CellsX(); ++x)
      {
        if (!edgeRow && x != 0 && x + 1 != patch.CellsX() && (!drawnRow || x < firstX || x > lastX))
          continue;
        // The triangles of RasterPatch::Triangle.
        uint32_t const sw = y * stride + x, ne = sw + stride + 1;
        m2::PointD const pointSW(xs[x], ys[y]), pointNE(xs[x + 1], ys[y + 1]);
        addFace({sw, sw + 1, ne}, pointSW, {pointNE.x, pointSW.y}, pointNE);
        addFace({sw, ne, ne - 1}, pointSW, pointNE, {pointSW.x, pointNE.y});
      }
    }
    RasterPerimeter const perimeter(patch.CellsX(), patch.CellsY());
    for (uint32_t i = 0; i < perimeter.Size(); ++i)
    {
      auto const node = perimeter(i);
      interface[impl::PointKey(patch.PointU(node))] += normals[node];
    }
  }
  // One lookup per TIN vertex: only the few seam vertices match. The mesh is deduplicated by the
  // same key, so one vertex completes its interface entry.
  for (size_t v = 0; v < result.m_tin.size(); ++v)
  {
    if (auto it = interface.find(impl::PointKey(mesh.GetPointU(v))); it != interface.end())
    {
      it->second += result.m_tin[v];
      result.m_tin[v] = it->second;
    }
  }
  for (size_t p = 0; p < data.m_rasterPatches.size(); ++p)
  {
    auto const & patch = data.m_rasterPatches[p];
    RasterPerimeter const perimeter(patch.CellsX(), patch.CellsY());
    for (uint32_t i = 0; i < perimeter.Size(); ++i)
    {
      auto const node = perimeter(i);
      result.m_raster[p][node] = interface.at(impl::PointKey(patch.PointU(node)));
    }
  }
  return result;
}
}  // namespace terrain
