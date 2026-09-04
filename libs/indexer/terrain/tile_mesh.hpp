#pragma once

#include "coding/point_coding.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "3party/skarupke/flat_hash_map.hpp"

namespace terrain
{
// ~0.037 m of a horizontal precision (kPointCoordBits, the mercator quantization limit):
// the terrain vertices cost the same at any bits count, they are coded as the lattice
// indices of their mesh (see terrain_serdes.hpp), not as the quantized points.
uint8_t constexpr kTerrainCoordBits = 30;

namespace impl
{
inline uint64_t PointKey(m2::PointU const & pt)
{
  return (static_cast<uint64_t>(pt.x) << 32) | pt.y;
}

// std::hash<uint64_t> is the identity on libc++, and the keys above are structured (the
// lattice mapped x in the high half, y in the low one), so the neighbour vertices of a mesh
// would land in the neighbour buckets and cluster. The splitmix64 finalizer costs 5 ops and
// spreads every input bit over the whole word.
struct PointKeyHash
{
  size_t operator()(uint64_t key) const
  {
    key ^= key >> 30;
    key *= 0xbf58476d1ce4e5b9ull;
    key ^= key >> 27;
    key *= 0x94d049bb133111ebull;
    key ^= key >> 31;
    return static_cast<size_t>(key);
  }
};
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
  // Inline: the geometry decoder calls it a couple of million times per block.
  uint32_t AddVertex(m2::PointU const & pt, int32_t altitude)
  {
    auto const [it, inserted] = m_pointToIndex.emplace(impl::PointKey(pt), static_cast<uint32_t>(m_points.size()));
    if (inserted)
    {
      m_points.push_back(PointUToPointD(pt, m_coordBits));
      m_altitudes.push_back(altitude);
    }
    return it->second;
  }

  // Makes room for |vertices| more deduplicated vertices (the dedup map included) and
  // |triangles| more triangles: the decoder knows a cheap upper estimate of every feature up
  // front, and the growth of the map and of the index vector dominates the decode. An over
  // estimate only wastes the transient capacity of one tile mesh.
  void ReserveAdditional(size_t vertices, size_t triangles);

  void AddTriangle(uint32_t a, uint32_t b, uint32_t c)
  {
    m_triangles.push_back(a);
    m_triangles.push_back(b);
    m_triangles.push_back(c);
  }

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
  ska::flat_hash_map<uint64_t, uint32_t, impl::PointKeyHash> m_pointToIndex;
};
}  // namespace terrain
