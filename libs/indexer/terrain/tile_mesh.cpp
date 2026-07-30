#include "indexer/terrain/tile_mesh.hpp"

#include "coding/point_coding.hpp"

namespace terrain
{
uint32_t TileMesh::AddVertex(m2::PointU const & pt, int32_t altitude)
{
  auto const [it, inserted] = m_pointToIndex.emplace(impl::PointKey(pt), static_cast<uint32_t>(m_points.size()));
  if (inserted)
  {
    m_points.push_back(PointUToPointD(pt, m_coordBits));
    m_altitudes.push_back(altitude);
  }
  return it->second;
}
}  // namespace terrain
