#pragma once

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/osm_id.hpp"

#include <cstdint>
#include <vector>

namespace indexer
{
// Class for intermediate objects used to build LocalityIndex.
class LocalityObject
{
public:
  // Decodes id stored in LocalityIndex. See GetStoredId().
  static osm::Id FromStoredId(uint64_t storedId) { return osm::Id(storedId >> 2 | storedId << 62); }

  // We need LocalityIndex object id to be at most numeric_limits<int64_t>::max().
  // We use incremental encoding for ids and need to keep ids of close object close if it is possible.
  // To ensure it we move two leading bits which encodes object type to the end of id.
  uint64_t GetStoredId() const { return m_id << 2 | m_id >> 62; }

  void Deserialize(char const * data);

  template <typename ToDo>
  void ForEachPoint(ToDo && toDo) const
  {
    for (auto const & p : m_points)
      toDo(p);
  }

  template <typename ToDo>
  void ForEachTriangle(ToDo && toDo) const
  {
    for (size_t i = 2; i < m_triangles.size(); i += 3)
      toDo(m_triangles[i - 2], m_triangles[i - 1], m_triangles[i]);
  }

  void SetForTests(uint64_t id, m2::PointD point)
  {
    m_id = id;
    m_points.clear();
    m_points.push_back(point);
  }

private:
  uint64_t m_id = 0;
  std::vector<m2::PointD> m_points;
  // m_triangles[3 * i], m_triangles[3 * i + 1], m_triangles[3 * i + 2] form the i-th triangle.
  buffer_vector<m2::PointD, 32> m_triangles;
};
}  // namespace indexer
