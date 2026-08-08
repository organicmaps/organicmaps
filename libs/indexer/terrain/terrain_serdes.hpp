#pragma once

#include "indexer/cell_coverer.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/cell_value_pair.hpp"
#include "indexer/interval_index_builder.hpp"
#include "indexer/terrain/tile_mesh.hpp"

#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/exception.hpp"

#include "defines.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <vector>

namespace terrain
{
using Altitude = geometry::Altitude;

// TWM (terrain) file format, version 2. A .twm file is a FilesContainer structured like an MWM:
// - kHeaderTag: version, the coordinate codec, coordinate bits, geometry scales with their max
//   vertical errors, the limit rect and the count of the meshes merged into the file. Parsed on
//   every registration (TwmSet::Register), so it stays small;
// - kGridTag: the lattice tables of every mesh (see MeshGrid), read once by terrain::Reader;
// - kFeaturesTag: FeatureRecords back to back, ordered by the feature center cell id
//   (nearby features lay nearby in the file, like MWM features). A record stores the
//   altitudes range, the feature limit rect, its mesh index and one offset per geometry scale;
// - kGeometryTags[g] ("trg0", "trg1", ...): triangle geometries of one scale, separated
//   per section (like MWM trgN), so rendering at one scale keeps a dense FilesContainer
//   cache instead of skipping the other scale blobs;
// - kIndexTag: an interval index over the RectId cells covering the feature rects,
//   mapping cell intervals to the FeatureRecord offsets (the MWM scale index scheme).
//
// The vertices lay on the geographic samples lattice of their mesh (the DEM grid, m_samplesPerDegree
// samples per degree on both axes), so a geometry codes mesh-local (column, row) integer pairs and
// the grid section maps them to the quantized mercator points. The mapping is separable - the
// mercator x depends on the longitude only and y on the latitude only - so (gridWidth + 1) +
// (gridHeight + 1) table values replace the per-vertex projection: the deltas of the small lattice
// ints are much cheaper than the deltas of the quantized points, the client needs no trigonometry,
// and a vertex shared by two features, two meshes or two files still decodes to the identical
// point - adjacency can be restored by hashing points.
//
// A TrianglesFeature geometry is a chunk of the terrain mesh serialized as DFS tree chains
// with the parallelogram point prediction - the serial::TrianglesChainSaver scheme of the
// MWM trg sections - extended with a per-vertex altitude channel:
// - a chain root is a triangle with an outer edge: the plain lattice coordinates of the first
//   vertex, then 2 per axis deltas per the other two vertices (a root delta spans the whole
//   feature, interleaving it as below would double the longer axis), and 3 altitudes (against
//   the feature min altitude, then the previous altitude);
// - every next triangle shares the (a, b) edge with its DFS parent (a, b, c) and takes one
//   varint carrying 3 tag bits - the tree bits shifted by the known vertex flag:
//   * a new vertex: (BitwiseMerge(zigzag dcolumn, zigzag drow) << 3 | treeBits << 1 | 0) plus one
//     varint altitude delta against the altitude prediction alt(a) + alt(b) - alt(c). The vertex
//     is predicted by the parallelogram rule from the shared edge (clamped to the lattice);
//   * a known vertex: (backreference << 3 | treeBits << 1 | 1), no coordinates and no altitude.
//     A triangulated disk has about twice as many triangles as vertices, so about a half of the
//     inner triangles close onto a vertex this geometry has already coded. Both sides count the
//     decoded vertex slots in the emission order - a chain root appends its 3 vertices (roots are
//     never deduplicated, the decoder can not tell a repeated corner from a new one), a new inner
//     vertex appends 1, a known one appends nothing - and the backreference is the distance from
//     the last appended slot;
// - 2 tree bits: bit 1 - the DFS continues across the (b, c) edge, bit 2 - a subtree hangs
//   across the (c, a) edge (pushed to the stack), no bits - return to the stack top or
//   finish the chain.

uint8_t constexpr kTwmVersion = 2;
// The cell depth of the geometry index, the same as the MWM scale index uses.
int constexpr kCellDepth = RectId::DEPTH_LEVELS;
size_t constexpr kMaxCellsPerFeature = 32;
// One .twm holds the meshes of one dynamic grid block, a handful of them in practice.
uint32_t constexpr kMaxMeshCount = 4096;

// Thrown on corrupt or unsupported .twm data. The decoding side must stay catchable
// on the client, unlike the always-aborting CHECKs of the encoding side.
DECLARE_EXCEPTION(TwmException, RootException);

char constexpr kHeaderTag[] = HEADER_FILE_TAG;
char constexpr kGridTag[] = "grid";
char constexpr kFeaturesTag[] = FEATURES_FILE_TAG;
char constexpr kIndexTag[] = "index";  /// @todo Make INDEX_FILE_TAG

inline std::string GetGeometryTag(size_t geomIndex)
{
  ASSERT_LESS(geomIndex, 10, ());
  return std::string(TRIANGLE_FILE_TAG) + static_cast<char>('0' + geomIndex);
}

// The coordinate codec of the geometry sections, a file level tag: the coordinates of a
// geometry blob mean nothing without it.
enum class CoordinateCodec : uint8_t
{
  // The only implemented one: mesh-local lattice indices mapped by the grid section tables.
  GeographicGrid = 0
};

struct TwmHeader
{
  struct Geometry
  {
    int8_t m_scale = 0;         // The upper zoom level served by this geometry.
    uint32_t m_maxErrorMm = 0;  // Max vertical error of the mesh, millimeters.
  };

  uint8_t m_version = kTwmVersion;
  CoordinateCodec m_codec = CoordinateCodec::GeographicGrid;
  uint8_t m_coordBits = kTerrainCoordBits;
  // Ordered from the coarsest to the finest.
  std::vector<Geometry> m_geometries;
  // The limit rect of all the TWM features, quantized.
  m2::PointU m_limitLB;
  m2::PointU m_limitRT;
  // The meshes merged into the file, one grid section entry each.
  uint32_t m_meshCount = 1;

  size_t GetGeometryIndex(int scale) const
  {
    ASSERT(!m_geometries.empty(), ());
    for (size_t i = 0; i + 1 < m_geometries.size(); ++i)
      if (scale <= m_geometries[i].m_scale)
        return i;
    return m_geometries.size() - 1;
  }

  m2::RectD GetLimitRect() const
  {
    return {PointUToPointD(m_limitLB, m_coordBits), PointUToPointD(m_limitRT, m_coordBits)};
  }

  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK(m_limitLB.x <= m_limitRT.x && m_limitLB.y <= m_limitRT.y, ());
    CHECK(m_meshCount > 0 && m_meshCount <= kMaxMeshCount, (m_meshCount));
    WriteToSink(sink, m_version);
    WriteToSink(sink, static_cast<uint8_t>(m_codec));
    WriteToSink(sink, m_coordBits);
    WriteToSink(sink, static_cast<uint8_t>(m_geometries.size()));
    for (auto const & geometry : m_geometries)
    {
      WriteToSink(sink, geometry.m_scale);
      WriteToSink(sink, geometry.m_maxErrorMm);
    }
    WriteVarUint(sink, m_limitLB.x);
    WriteVarUint(sink, m_limitLB.y);
    WriteVarUint(sink, m_limitRT.x - m_limitLB.x);
    WriteVarUint(sink, m_limitRT.y - m_limitLB.y);
    WriteVarUint(sink, m_meshCount);
  }

  template <typename Source>
  void Deserialize(Source & src)
  {
    m_version = ReadPrimitiveFromSource<uint8_t>(src);
    if (m_version != kTwmVersion)
      MYTHROW(TwmException, ("Unsupported TWM version", m_version));
    auto const codec = ReadPrimitiveFromSource<uint8_t>(src);
    if (codec != static_cast<uint8_t>(CoordinateCodec::GeographicGrid))
      MYTHROW(TwmException, ("Unsupported TWM coordinate codec", codec));
    m_codec = static_cast<CoordinateCodec>(codec);
    m_coordBits = ReadPrimitiveFromSource<uint8_t>(src);
    if (m_coordBits == 0 || m_coordBits > kPointCoordBits)
      MYTHROW(TwmException, ("Invalid coord bits", m_coordBits));
    m_geometries.resize(ReadPrimitiveFromSource<uint8_t>(src));
    if (m_geometries.empty())
      MYTHROW(TwmException, ("Empty geometries list"));
    for (auto & geometry : m_geometries)
    {
      geometry.m_scale = ReadPrimitiveFromSource<int8_t>(src);
      geometry.m_maxErrorMm = ReadPrimitiveFromSource<uint32_t>(src);
    }
    m_limitLB.x = ReadVarUint<uint32_t>(src);
    m_limitLB.y = ReadVarUint<uint32_t>(src);
    m_limitRT.x = m_limitLB.x + ReadVarUint<uint32_t>(src);
    m_limitRT.y = m_limitLB.y + ReadVarUint<uint32_t>(src);
    m_meshCount = ReadVarUint<uint32_t>(src);
    if (m_meshCount == 0 || m_meshCount > kMaxMeshCount)
      MYTHROW(TwmException, ("Invalid meshes count", m_meshCount));
  }
};

// The samples lattice of one mesh: the quantized mercator coordinates of its columns and rows.
// A geometry codes the (column, row) index pairs, this table turns them back into the points.
// The tables are built with the canonical formula (docs/TERRAIN.md) and frozen into the file,
// so the borders of the neighbor meshes and blocks - the same physical parallels and meridians -
// map to the identical points.
struct MeshGrid
{
  // The mesh south-west corner in the global lattice units, kept for the tools and the
  // diagnostics: the decoder addresses the tables directly.
  int32_t m_originLatSamples = 0;
  int32_t m_originLonSamples = 0;
  uint32_t m_samplesPerDegree = 0;
  // Quantized mercator x per lattice column and y per lattice row, strictly increasing.
  std::vector<uint32_t> m_colX;
  std::vector<uint32_t> m_rowY;

  // The lattice coordinates are (column, row), the max valid pair.
  m2::PointU GetMax() const
  {
    ASSERT(!m_colX.empty() && !m_rowY.empty(), ());
    return {static_cast<uint32_t>(m_colX.size() - 1), static_cast<uint32_t>(m_rowY.size() - 1)};
  }

  m2::PointU GetPoint(m2::PointU const & lattice) const
  {
    ASSERT(lattice.x < m_colX.size() && lattice.y < m_rowY.size(), (lattice, m_colX.size(), m_rowY.size()));
    return {m_colX[lattice.x], m_rowY[lattice.y]};
  }
};

namespace impl
{
// A strictly increasing table, delta of delta coded: the lattice step of the mercator
// projection changes slowly, so the second differences are tiny (one byte varints).
template <typename Sink>
void SerializeTable(Sink & sink, std::vector<uint32_t> const & table, uint8_t coordBits)
{
  CHECK(!table.empty(), ());
  CHECK_LESS(table.back(), uint64_t{1} << coordBits, ());
  WriteVarUint(sink, table.front());
  if (table.size() == 1)
    return;

  CHECK_LESS(table[0], table[1], ());
  int64_t prevDelta = int64_t{table[1]} - table[0];
  WriteVarUint(sink, static_cast<uint32_t>(prevDelta));
  for (size_t i = 2; i < table.size(); ++i)
  {
    CHECK_LESS(table[i - 1], table[i], (i));
    int64_t const delta = int64_t{table[i]} - table[i - 1];
    WriteVarInt(sink, delta - prevDelta);
    prevDelta = delta;
  }
}

template <typename Source>
void DeserializeTable(Source & src, size_t count, uint8_t coordBits, std::vector<uint32_t> & table)
{
  ASSERT_GREATER(count, 0, ());
  auto const limit = static_cast<int64_t>(uint64_t{1} << coordBits);

  table.resize(count);
  int64_t value = ReadVarUint<uint32_t>(src);
  if (value >= limit)
    MYTHROW(TwmException, ("Grid table value out of the coordinate range", value, limit));
  table[0] = static_cast<uint32_t>(value);
  if (count == 1)
    return;

  int64_t delta = ReadVarUint<uint32_t>(src);
  for (size_t i = 1; i < count; ++i)
  {
    if (i > 1)
    {
      // A valid delta is under the limit, so the second difference is bounded by it too:
      // the check also keeps the accumulation below from overflowing.
      int64_t const secondDelta = ReadVarInt<int64_t>(src);
      if (secondDelta < -limit || secondDelta > limit)
        MYTHROW(TwmException, ("Corrupt grid table delta", i, secondDelta));
      delta += secondDelta;
    }
    if (delta <= 0)
      MYTHROW(TwmException, ("Non increasing grid table", i, delta));
    value += delta;
    if (value >= limit)
      MYTHROW(TwmException, ("Grid table value out of the coordinate range", i, value, limit));
    table[i] = static_cast<uint32_t>(value);
  }
}
}  // namespace impl

// The grid section: the lattice tables of every mesh of the file, in the mesh index order.
template <typename Sink>
void SerializeGrid(Sink & sink, std::vector<MeshGrid> const & grids, uint8_t coordBits)
{
  CHECK(!grids.empty() && grids.size() <= kMaxMeshCount, (grids.size()));
  for (auto const & grid : grids)
  {
    CHECK(!grid.m_colX.empty() && !grid.m_rowY.empty(), ());
    CHECK_GREATER(grid.m_samplesPerDegree, 0, ());
    WriteVarInt(sink, grid.m_originLatSamples);
    WriteVarInt(sink, grid.m_originLonSamples);
    WriteVarUint(sink, grid.m_samplesPerDegree);
    WriteVarUint(sink, grid.GetMax().x);
    WriteVarUint(sink, grid.GetMax().y);
    impl::SerializeTable(sink, grid.m_rowY, coordBits);
    impl::SerializeTable(sink, grid.m_colX, coordBits);
  }
}

template <typename Source>
void DeserializeGrid(Source & src, TwmHeader const & header, std::vector<MeshGrid> & grids)
{
  grids.clear();
  grids.resize(header.m_meshCount);
  for (auto & grid : grids)
  {
    grid.m_originLatSamples = ReadVarInt<int32_t>(src);
    grid.m_originLonSamples = ReadVarInt<int32_t>(src);
    grid.m_samplesPerDegree = ReadVarUint<uint32_t>(src);
    if (grid.m_samplesPerDegree == 0)
      MYTHROW(TwmException, ("Invalid mesh samples per degree"));

    uint64_t const cols = uint64_t{ReadVarUint<uint32_t>(src)} + 1;
    uint64_t const rows = uint64_t{ReadVarUint<uint32_t>(src)} + 1;
    // Every table value takes at least one byte: reject the sizes the section can not hold
    // before allocating them.
    if (cols + rows > src.Size())
      MYTHROW(TwmException, ("Corrupt grid tables sizes", cols, rows, src.Size()));

    impl::DeserializeTable(src, rows, header.m_coordBits, grid.m_rowY);
    impl::DeserializeTable(src, cols, header.m_coordBits, grid.m_colX);
  }
}

// The FeatureRecord of the features section: everything needed to cull a feature and locate
// its geometries. The geometry offsets are absolute (the records are accessed randomly via
// the index).
struct FeatureRecord
{
  uint32_t m_meshIdx = 0;  // The mesh of the feature vertices, an index into the grid section.
  m2::PointU m_rectLB;
  m2::PointU m_rectRT;
  Altitude m_minAltitude = 0;
  Altitude m_maxAltitude = 0;
  std::vector<uint64_t> m_geomOffsets;  // One offset into the kGeometryTags[g] section per geometry.

  m2::RectD GetRect(uint8_t coordBits) const
  {
    return {PointUToPointD(m_rectLB, coordBits), PointUToPointD(m_rectRT, coordBits)};
  }
};

template <typename Sink>
void SerializeFeatureRecord(Sink & sink, FeatureRecord const & record, TwmHeader const & header)
{
  CHECK_EQUAL(record.m_geomOffsets.size(), header.m_geometries.size(), ());
  CHECK_LESS(record.m_meshIdx, header.m_meshCount, ());
  CHECK(record.m_rectLB.x >= header.m_limitLB.x && record.m_rectLB.y >= header.m_limitLB.y &&
            record.m_rectRT.x <= header.m_limitRT.x && record.m_rectRT.y <= header.m_limitRT.y,
        ());
  CHECK(record.m_rectLB.x <= record.m_rectRT.x && record.m_rectLB.y <= record.m_rectRT.y, ());
  CHECK_LESS_OR_EQUAL(record.m_minAltitude, record.m_maxAltitude, ());

  WriteVarUint(sink, record.m_meshIdx);
  WriteVarInt(sink, static_cast<int32_t>(record.m_minAltitude));
  WriteVarUint(sink, static_cast<uint32_t>(record.m_maxAltitude - record.m_minAltitude));
  WriteVarUint(sink, record.m_rectLB.x - header.m_limitLB.x);
  WriteVarUint(sink, record.m_rectLB.y - header.m_limitLB.y);
  WriteVarUint(sink, record.m_rectRT.x - record.m_rectLB.x);
  WriteVarUint(sink, record.m_rectRT.y - record.m_rectLB.y);
  for (uint64_t const offset : record.m_geomOffsets)
    WriteVarUint(sink, offset);
}

template <typename Source>
void DeserializeFeatureRecord(Source & src, TwmHeader const & header, FeatureRecord & record)
{
  record.m_meshIdx = ReadVarUint<uint32_t>(src);
  if (record.m_meshIdx >= header.m_meshCount)
    MYTHROW(TwmException, ("Feature mesh index out of range", record.m_meshIdx, header.m_meshCount));

  int32_t const minAltitude = ReadVarInt<int32_t>(src);
  record.m_minAltitude = static_cast<Altitude>(minAltitude);
  int64_t const maxAltitude = record.m_minAltitude + int64_t{ReadVarUint<uint32_t>(src)};
  record.m_maxAltitude = static_cast<Altitude>(maxAltitude);

  uint64_t const lbx = header.m_limitLB.x + uint64_t{ReadVarUint<uint32_t>(src)};
  uint64_t const lby = header.m_limitLB.y + uint64_t{ReadVarUint<uint32_t>(src)};
  uint64_t const rtx = lbx + ReadVarUint<uint32_t>(src);
  uint64_t const rty = lby + ReadVarUint<uint32_t>(src);
  record.m_rectLB = {static_cast<uint32_t>(lbx), static_cast<uint32_t>(lby)};
  record.m_rectRT = {static_cast<uint32_t>(rtx), static_cast<uint32_t>(rty)};

  record.m_geomOffsets.resize(header.m_geometries.size());
  for (auto & offset : record.m_geomOffsets)
    offset = ReadVarUint<uint64_t>(src);
}

// Encoder input: the vertices of one mesh deduplicated within the feature - both as the
// lattice (column, row) pairs the geometry is coded in and as the quantized points the
// feature rect is built from - and CCW vertex index triples per geometry scale.
struct FeatureData
{
  std::vector<m2::PointU> m_lattice;
  std::vector<m2::PointU> m_points;
  std::vector<Altitude> m_altitudes;
  std::vector<std::vector<uint32_t>> m_geomTriangles;

  void GetAltitudesRange(Altitude & minAltitude, Altitude & maxAltitude) const
  {
    ASSERT(!m_altitudes.empty(), ());
    auto const [minIt, maxIt] = std::minmax_element(m_altitudes.begin(), m_altitudes.end());
    minAltitude = *minIt;
    maxAltitude = *maxIt;
  }
};

namespace impl
{
uint32_t constexpr kNoTri = std::numeric_limits<uint32_t>::max();
uint32_t constexpr kNoSlot = std::numeric_limits<uint32_t>::max();
// The low bit of an inner triangle event: its vertex is a backreference to an already
// emitted one instead of a new coordinates and altitude pair.
uint64_t constexpr kKnownVertexTag = 1;

inline uint64_t EdgeKey(uint32_t u, uint32_t v)
{
  return (static_cast<uint64_t>(u) << 32) | v;
}

// The parallelogram prediction of coding::PredictPointInTriangle in the lattice ints:
// the new vertex of the (u, v) edge continues the (pred, u, v) triangle. Clamping keeps
// the prediction a valid lattice pair and the deltas bounded by the lattice size (which
// is under 2^30 - the table values are strictly increasing and fit the coordinate bits -
// so the interleaved zigzag deltas always fit the 61 bits payload of an inner event).
inline m2::PointU PredictLattice(m2::PointU const & u, m2::PointU const & v, m2::PointU const & pred,
                                 m2::PointU const & maxLattice)
{
  auto const predict = [](int64_t value, uint32_t maxValue)
  { return static_cast<uint32_t>(std::clamp<int64_t>(value, 0, maxValue)); };
  return {predict(int64_t{u.x} + v.x - pred.x, maxLattice.x), predict(int64_t{u.y} + v.y - pred.y, maxLattice.y)};
}

// The decoded lattice pair is a pair of table indices: validate before the lookup. A valid
// stream never throws here - the roots are coded plainly and the inner vertices land exactly
// on the encoder ones, inside the lattice.
inline m2::PointU CheckLattice(int64_t col, int64_t row, m2::PointU const & maxLattice)
{
  if (col < 0 || col > maxLattice.x || row < 0 || row > maxLattice.y)
    MYTHROW(TwmException, ("Vertex out of the mesh lattice", col, row, maxLattice));
  return {static_cast<uint32_t>(col), static_cast<uint32_t>(row)};
}

// DFS tree chains encoder, the scheme of serial::TrianglesChainSaver extended with altitudes.
class ChainEncoder
{
public:
  ChainEncoder(FeatureData const & data, std::vector<uint32_t> const & triangles, m2::PointU const & maxLattice,
               Altitude minAltitude)
    : m_data(data)
    , m_triangles(triangles)
    , m_maxLattice(maxLattice)
    , m_minAltitude(minAltitude)
  {
    size_t const count = triangles.size() / 3;
    m_edgeToTri.reserve(count * 3);
    for (uint32_t t = 0; t < count; ++t)
    {
      for (size_t e = 0; e < 3; ++e)
      {
        bool const inserted = m_edgeToTri.emplace(EdgeKey(Vertex(t, e), Vertex(t, (e + 1) % 3)), t).second;
        CHECK(inserted, ("Inconsistent triangles orientation", t));
      }
    }
    m_visited.assign(count, false);
    m_slotOf.assign(data.m_lattice.size(), kNoSlot);
  }

  template <typename Sink>
  void Save(Sink & sink)
  {
    size_t const count = m_triangles.size() / 3;
    WriteVarUint(sink, static_cast<uint64_t>(count));

    size_t emitted = 0;
    while (emitted < count)
    {
      uint32_t root = kNoTri;
      size_t rootEdge = 0;
      for (uint32_t t = 0; t < count && root == kNoTri; ++t)
      {
        if (m_visited[t])
          continue;
        for (size_t e = 0; e < 3; ++e)
        {
          // The chain root must be entered via an outer edge: the hull or a visited neighbor.
          uint32_t const nb = Neighbor(Vertex(t, e), Vertex(t, (e + 1) % 3));
          if (nb == kNoTri || m_visited[nb])
          {
            root = t;
            rootEdge = e;
            break;
          }
        }
      }
      CHECK(root != kNoTri, ("No chain root found", emitted, count));
      emitted += EmitChain(sink, root, rootEdge);
    }
  }

private:
  struct Ctx
  {
    uint32_t m_tri;
    uint32_t m_u, m_v;  // The edge shared with the parent, directed as in this triangle.
    uint32_t m_pred;    // The parent vertex opposite the shared edge: the parallelogram prediction base.
  };

  uint32_t Vertex(uint32_t tri, size_t i) const { return m_triangles[tri * 3 + i]; }
  m2::PointU const & Lattice(uint32_t v) const { return m_data.m_lattice[v]; }
  int32_t Alt(uint32_t v) const { return m_data.m_altitudes[v]; }

  static int32_t Delta(uint32_t value, uint32_t base) { return static_cast<int32_t>(int64_t{value} - base); }

  uint32_t Neighbor(uint32_t u, uint32_t v) const
  {
    auto const it = m_edgeToTri.find(EdgeKey(v, u));
    return it == m_edgeToTri.end() ? kNoTri : it->second;
  }

  // An event varint: the payload above kShift tag bits - 2 tree bits for a chain root,
  // plus the known vertex flag for an inner triangle.
  template <uint8_t kShift, typename Sink>
  void WritePacked(Sink & sink, uint64_t payload, uint64_t tag)
  {
    CHECK_EQUAL(payload >> (64 - kShift), 0, ());
    WriteVarUint(sink, (payload << kShift) | tag);
  }

  // Appends a decoded vertex slot, exactly where the decoder appends one. A vertex re-emitted
  // by a later chain of this geometry overwrites its slot: the latest one wins, any consistent
  // policy works because a backreference is a numeric slot distance, not a vertex id.
  void AddSlot(uint32_t v) { m_slotOf[v] = m_slotCount++; }

  // Computes tree bits of the (a, b, c) triangle and marks its children visited.
  // Children must be committed at the emission time, otherwise another DFS branch
  // could claim them first and desynchronize the decoder.
  uint8_t TreeBits(uint32_t a, uint32_t b, uint32_t c, Ctx & child0, Ctx & child1)
  {
    uint8_t bits = 0;
    uint32_t const nb0 = Neighbor(b, c);
    if (nb0 != kNoTri && !m_visited[nb0])
    {
      m_visited[nb0] = true;
      child0 = {nb0, c, b, a};
      bits |= 1;
    }
    uint32_t const nb1 = Neighbor(c, a);
    if (nb1 != kNoTri && !m_visited[nb1])
    {
      m_visited[nb1] = true;
      child1 = {nb1, a, c, b};
      bits |= 2;
    }
    return bits;
  }

  template <typename Sink>
  size_t EmitChain(Sink & sink, uint32_t root, size_t rootEdge)
  {
    m_visited[root] = true;
    uint32_t const a = Vertex(root, rootEdge);
    uint32_t const b = Vertex(root, (rootEdge + 1) % 3);
    uint32_t const c = Vertex(root, (rootEdge + 2) % 3);

    Ctx child0, child1;
    uint8_t bits = TreeBits(a, b, c, child0, child1);
    auto const & p0 = Lattice(a);
    auto const & p1 = Lattice(b);
    auto const & p2 = Lattice(c);
    WriteVarUint(sink, p0.x);
    WriteVarUint(sink, p0.y);
    WriteVarInt(sink, Delta(p1.x, p0.x));
    WriteVarInt(sink, Delta(p1.y, p0.y));
    WriteVarInt(sink, Delta(p2.x, p1.x));
    WritePacked<2>(sink, bits::ZigZagEncode(Delta(p2.y, p1.y)), bits);
    WriteVarUint(sink, static_cast<uint32_t>(Alt(a) - m_minAltitude));
    WriteVarInt(sink, Alt(b) - Alt(a));
    WriteVarInt(sink, Alt(c) - Alt(b));
    AddSlot(a);
    AddSlot(b);
    AddSlot(c);

    size_t emitted = 1;
    std::vector<Ctx> stack;
    while (true)
    {
      Ctx cur;
      if (bits & 1)
      {
        if (bits & 2)
          stack.push_back(child1);
        cur = child0;
      }
      else if (bits & 2)
      {
        cur = child1;
      }
      else if (!stack.empty())
      {
        cur = stack.back();
        stack.pop_back();
      }
      else
      {
        break;
      }

      // The new vertex is the one not on the shared edge.
      uint32_t w = kNoTri;
      for (size_t i = 0; i < 3; ++i)
      {
        uint32_t const v = Vertex(cur.m_tri, i);
        if (v != cur.m_u && v != cur.m_v)
        {
          w = v;
          break;
        }
      }
      CHECK(w != kNoTri, (cur.m_tri));

      bits = TreeBits(cur.m_u, cur.m_v, w, child0, child1);
      uint32_t const slot = m_slotOf[w];
      if (slot != kNoSlot)
      {
        WritePacked<3>(sink, m_slotCount - 1 - slot, (uint64_t{bits} << 1) | kKnownVertexTag);
      }
      else
      {
        auto const prediction = PredictLattice(Lattice(cur.m_u), Lattice(cur.m_v), Lattice(cur.m_pred), m_maxLattice);
        WritePacked<3>(sink,
                       bits::BitwiseMerge(bits::ZigZagEncode(Delta(Lattice(w).x, prediction.x)),
                                          bits::ZigZagEncode(Delta(Lattice(w).y, prediction.y))),
                       uint64_t{bits} << 1);
        WriteVarInt(sink, Alt(w) - (Alt(cur.m_u) + Alt(cur.m_v) - Alt(cur.m_pred)));
        AddSlot(w);
      }
      ++emitted;
    }
    return emitted;
  }

  FeatureData const & m_data;
  std::vector<uint32_t> const & m_triangles;
  m2::PointU const m_maxLattice;
  int32_t const m_minAltitude;
  std::unordered_map<uint64_t, uint32_t> m_edgeToTri;
  std::vector<bool> m_visited;
  // The emitted vertex slots: the count of them and the last slot of every vertex, kNoSlot
  // until the vertex is coded for the first time.
  uint32_t m_slotCount = 0;
  std::vector<uint32_t> m_slotOf;
};

// Appends the decoded feature chains straight into the tile mesh: the vertices dedup
// globally there (one map per tile, no per-feature maps or intermediate vectors). The
// chain walk state stays feature-local: the codec point and altitude predictions must
// use this feature's OWN decoded values, which may differ from the mesh ones on a
// border vertex shared with an already added block of another data version.
template <typename Source>
void DecodeChains(Source & src, MeshGrid const & grid, Altitude minAltitude, TileMesh & mesh)
{
  uint64_t const count = ReadVarUint<uint64_t>(src);
  // Every encoded triangle takes at least one byte (a known vertex event is a single small
  // varint): fail on a corrupt count early, before it drives the decode loop. The mesh
  // triangles vector grows amortized, an exact per-feature reserve would realloc the
  // accumulated indices on every feature.
  if (count > src.Size())
    MYTHROW(TwmException, ("Corrupt triangles count", count));

  m2::PointU const maxLattice = grid.GetMax();

  // The emitted vertex slots: the feature-local prediction sources, NOT read back from the
  // mesh (the altitude deltas were encoded against this feature's own values - a shared
  // border vertex may keep another block's altitude in the mesh - and the point prediction
  // needs the lattice coordinates the mesh does not store), and the list the known vertex
  // backreferences address. A vertex the encoder emitted twice (chains sharing a corner)
  // takes two slots with identical values - the mesh index is the same for both.
  std::vector<m2::PointU> lattice;
  std::vector<int32_t> altitudes;
  std::vector<uint32_t> meshIndex;

  auto const addVertex = [&](m2::PointU const & pt, int64_t alt) -> uint32_t
  {
    lattice.push_back(pt);
    altitudes.push_back(static_cast<int32_t>(alt));
    meshIndex.push_back(mesh.AddVertex(grid.GetPoint(pt), static_cast<int32_t>(alt)));
    return static_cast<uint32_t>(lattice.size() - 1);
  };

  // Not named "emit" to keep the header compatible with the Qt keyword macros.
  auto const emitTriangle = [&](uint32_t a, uint32_t b, uint32_t c)
  { mesh.AddTriangle(meshIndex[a], meshIndex[b], meshIndex[c]); };

  struct Ctx
  {
    uint32_t m_u, m_v, m_pred;
  };
  std::vector<Ctx> stack;

  size_t decoded = 0;
  while (decoded < count)
  {
    // A new chain root, per axis deltas (the reads are ordered, not function arguments).
    uint32_t const col0 = ReadVarUint<uint32_t>(src);
    uint32_t const row0 = ReadVarUint<uint32_t>(src);
    m2::PointU const p0 = CheckLattice(col0, row0, maxLattice);
    int32_t const dcol1 = ReadVarInt<int32_t>(src);
    int32_t const drow1 = ReadVarInt<int32_t>(src);
    m2::PointU const p1 = CheckLattice(int64_t{p0.x} + dcol1, int64_t{p0.y} + drow1, maxLattice);
    int32_t const dcol2 = ReadVarInt<int32_t>(src);
    uint64_t const packed = ReadVarUint<uint64_t>(src);
    uint8_t bits = packed & 3;
    m2::PointU const p2 =
        CheckLattice(int64_t{p1.x} + dcol2, int64_t{p1.y} + bits::ZigZagDecode(packed >> 2), maxLattice);
    int64_t const z0 = minAltitude + int64_t{ReadVarUint<uint32_t>(src)};
    int64_t const z1 = z0 + ReadVarInt<int32_t>(src);
    int64_t const z2 = z1 + ReadVarInt<int32_t>(src);

    uint32_t pa = addVertex(p0, z0), pb = addVertex(p1, z1), pc = addVertex(p2, z2);
    emitTriangle(pa, pb, pc);
    ++decoded;

    ASSERT(stack.empty(), ());
    while (true)
    {
      Ctx cur;
      if (bits & 2)
        stack.emplace_back(pa, pc, pb);
      if (bits & 1)
      {
        cur = {pc, pb, pa};
      }
      else if (!stack.empty())
      {
        cur = stack.back();
        stack.pop_back();
      }
      else
      {
        break;
      }

      uint64_t const next = ReadVarUint<uint64_t>(src);
      bits = (next >> 1) & 3;
      uint32_t w;
      if (next & kKnownVertexTag)
      {
        // A vertex this geometry has already decoded: it takes a slot of the emission list,
        // with its lattice pair, altitude and mesh index, and appends no new one.
        uint64_t const backref = next >> 3;
        if (backref >= lattice.size())
          MYTHROW(TwmException, ("Vertex backreference out of the decoded slots", backref, lattice.size()));
        w = static_cast<uint32_t>(lattice.size() - 1 - backref);
      }
      else
      {
        uint32_t zigzagCol = 0, zigzagRow = 0;
        bits::BitwiseSplit(next >> 3, zigzagCol, zigzagRow);
        auto const prediction = PredictLattice(lattice[cur.m_u], lattice[cur.m_v], lattice[cur.m_pred], maxLattice);
        m2::PointU const pw = CheckLattice(int64_t{prediction.x} + bits::ZigZagDecode(zigzagCol),
                                           int64_t{prediction.y} + bits::ZigZagDecode(zigzagRow), maxLattice);
        int64_t const zw =
            int64_t{altitudes[cur.m_u]} + altitudes[cur.m_v] - altitudes[cur.m_pred] + ReadVarInt<int32_t>(src);
        w = addVertex(pw, zw);
      }
      emitTriangle(cur.m_u, cur.m_v, w);
      ++decoded;
      ASSERT_LESS_OR_EQUAL(decoded, count, ());
      pa = cur.m_u;
      pb = cur.m_v;
      pc = w;
    }
  }
}
}  // namespace impl

// Serializes one geometry of a feature into its trgN section sink:
// varuint triangles count + the DFS chains (see the format comment above).
template <typename Sink>
void SerializeFeatureGeometry(Sink & sink, FeatureData const & data, std::vector<uint32_t> const & triangles,
                              MeshGrid const & grid, Altitude minAltitude)
{
  CHECK(!data.m_lattice.empty(), ());
  CHECK_EQUAL(data.m_lattice.size(), data.m_altitudes.size(), ());
  CHECK_EQUAL(data.m_lattice.size(), data.m_points.size(), ());

  impl::ChainEncoder encoder(data, triangles, grid.GetMax(), minAltitude);
  encoder.Save(sink);
}

// Decodes one geometry located by a FeatureRecord into the tile mesh. The record
// supplies the altitudes base; the grid of the record mesh maps the lattice coordinates.
template <typename Source>
void DeserializeFeatureGeometry(Source & src, MeshGrid const & grid, FeatureRecord const & record, TileMesh & mesh)
{
  impl::DecodeChains(src, grid, record.m_minAltitude, mesh);
}

// Builds the geometry index section: an interval index over the RectId cells covering
// the feature rects, mapping cells to the FeatureRecord offsets (the MWM scale index scheme).
template <typename Sink>
void BuildIndex(Sink & sink, std::vector<std::pair<m2::RectD, uint32_t>> const & features)
{
  std::vector<covering::CellValuePair<uint32_t>> cellsToFeatures;
  for (auto const & [rect, offset] : features)
  {
    std::vector<RectId> cells;
    CoverRect<mercator::Bounds, RectId>(rect, kMaxCellsPerFeature, kCellDepth - 1, cells);
    CHECK(!cells.empty(), (rect));
    for (auto const & cell : cells)
      cellsToFeatures.emplace_back(cell.ToInt64(kCellDepth), offset);
  }
  std::sort(cellsToFeatures.begin(), cellsToFeatures.end());
  BuildIntervalIndex(cellsToFeatures.begin(), cellsToFeatures.end(), sink, kCellDepth * 2 + 1);
}
}  // namespace terrain
