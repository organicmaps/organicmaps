#include "indexer/terrain/tile_mesh.hpp"

#include <algorithm>

namespace terrain
{
void TileMesh::ReserveAdditional(size_t vertices, size_t triangles)
{
  // std::vector::reserve fits exactly, so reserving the running total once per feature would
  // realloc and copy on every one of them: keep the geometric growth of the push_back path.
  size_t const totalPoints = m_points.size() + vertices;
  if (totalPoints > m_points.capacity())
  {
    size_t const capacity = std::max(totalPoints, 2 * m_points.capacity());
    m_points.reserve(capacity);
    m_altitudes.reserve(capacity);
  }

  size_t const totalIndices = m_triangles.size() + 3 * triangles;
  if (totalIndices > m_triangles.capacity())
    m_triangles.reserve(std::max(totalIndices, 2 * m_triangles.capacity()));

  // Grow geometrically and never shrink: on 32-bit ska::flat_hash_map is std::unordered_map,
  // whose reserve() rehashes to the exact fit and may shrink.
  if (totalPoints > m_pointToIndex.bucket_count() * m_pointToIndex.max_load_factor())
    m_pointToIndex.reserve(std::max(totalPoints, 2 * m_pointToIndex.size()));
}
}  // namespace terrain
