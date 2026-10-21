#include "indexer/terrain/terrain_normals.hpp"

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
  return result;
}
}  // namespace terrain
