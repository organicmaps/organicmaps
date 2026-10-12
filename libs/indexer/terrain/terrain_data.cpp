#include "indexer/terrain/terrain_data.hpp"

#include <iterator>
#include <vector>

namespace terrain
{
void TerrainData::Append(TerrainData && other)
{
  ASSERT_EQUAL(m_mesh.GetCoordBits(), other.m_mesh.GetCoordBits(), ());
  if (IsEmpty())
  {
    *this = std::move(other);
    return;
  }

  auto const & triangles = other.m_mesh.GetTriangles();
  std::vector<uint32_t> indices;
  indices.reserve(other.m_mesh.GetPoints().size());

  m_mesh.ReserveAdditional(other.m_mesh.GetPoints().size(), other.m_mesh.GetTrianglesCount());
  for (size_t i = 0; i < other.m_mesh.GetPoints().size(); ++i)
    indices.push_back(m_mesh.AddVertex(other.m_mesh.GetPointU(i), other.m_mesh.GetAltitudes()[i]));

  for (size_t t = 0; t < triangles.size(); t += 3)
    m_mesh.AddTriangle(indices[triangles[t]], indices[triangles[t + 1]], indices[triangles[t + 2]]);

  m_rasterPatches.insert(m_rasterPatches.end(), std::make_move_iterator(other.m_rasterPatches.begin()),
                         std::make_move_iterator(other.m_rasterPatches.end()));
}
}  // namespace terrain
