#include "indexer/terrain/terrain_normals.hpp"

#include "3party/skarupke/flat_hash_map.hpp"

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
}  // namespace

TerrainNormals CalculateTerrainNormals(TerrainData const & data, double zScale,
                                       base::SharedCancellable const & isCancellable)
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
    for (uint32_t t = 0; t < 2 * patch.CellsX() * patch.CellsY(); ++t)
    {
      auto const v = patch.Triangle(t);
      auto const n = Face(patch.Point(v[0]), patch.Point(v[1]), patch.Point(v[2]), patch.m_heights[v[0]],
                          patch.m_heights[v[1]], patch.m_heights[v[2]], zScale);
      for (auto const node : v)
        normals[node] += n;
    }
    RasterPerimeter const perimeter(patch.CellsX(), patch.CellsY());
    for (uint32_t i = 0; i < perimeter.Size(); ++i)
    {
      auto const node = perimeter(i);
      interface[impl::PointKey(patch.PointU(node))] += normals[node];
    }
  }
  for (size_t v = 0; v < result.m_tin.size(); ++v)
    if (auto it = interface.find(impl::PointKey(mesh.GetPointU(v))); it != interface.end())
      it->second += result.m_tin[v];
  for (size_t v = 0; v < result.m_tin.size(); ++v)
    if (auto it = interface.find(impl::PointKey(mesh.GetPointU(v))); it != interface.end())
      result.m_tin[v] = it->second;
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
