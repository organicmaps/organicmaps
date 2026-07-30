#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace terrain
{
// ~0.3 m of a horizontal precision, enough against the ~30 m DEM samples step.
uint8_t constexpr kTerrainCoordBits = 27;

namespace impl
{
inline uint64_t PointKey(m2::PointU const & pt)
{
  return (static_cast<uint64_t>(pt.x) << 32) | pt.y;
}
}  // namespace impl

// The terrain mesh of one drawn tile, merged from the decoded features of all the
// intersecting TWM blocks. The vertices are deduplicated by the quantized keys (the
// quantization is against the global mercator bounds, so shared vertices of features
// and blocks carry identical m2::PointU), and every consumer - the hillshading
// normals, the isolines marching, the debug mesh - sews seamlessly over the single
// indexed mesh. Filled directly by the geometry decoder (see DecodeChains), one dedup
// map per tile; built once per tile read, see TerrainProvider::ReadMesh and
// RuleDrawer::DrawTerrain.
class TileMesh
{
public:
  explicit TileMesh(uint8_t coordBits = kTerrainCoordBits) : m_coordBits(coordBits) {}

  uint8_t GetCoordBits() const { return m_coordBits; }

  // Returns the index of the vertex, deduplicated by the quantized point. A vertex
  // shared with an already added block of another data version may carry a different
  // altitude there: the first added one wins deterministically.
  uint32_t AddVertex(m2::PointU const & pt, int32_t altitude);

  void AddTriangle(uint32_t a, uint32_t b, uint32_t c) { m_triangles.insert(m_triangles.end(), {a, b, c}); }

  bool IsEmpty() const { return m_triangles.empty(); }
  size_t GetTrianglesCount() const { return m_triangles.size() / 3; }

  std::vector<m2::PointD> const & GetPoints() const { return m_points; }
  // The altitudes are meters (the storage units), per deduplicated vertex.
  std::vector<int32_t> const & GetAltitudes() const { return m_altitudes; }
  // CCW vertex index triples over the deduplicated vertices.
  std::vector<uint32_t> const & GetTriangles() const { return m_triangles; }

private:
  uint8_t m_coordBits;

  std::vector<m2::PointD> m_points;
  std::vector<int32_t> m_altitudes;
  std::vector<uint32_t> m_triangles;

  // The build state: quantized point key -> the deduplicated vertex index.
  std::unordered_map<uint64_t, uint32_t> m_pointToIndex;
};
}  // namespace terrain
