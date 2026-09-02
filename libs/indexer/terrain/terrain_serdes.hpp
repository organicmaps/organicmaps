#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/terrain/tile_mesh.hpp"

#include "coding/endianness.hpp"
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
#include "base/macros.hpp"

#include "defines.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace terrain
{
using Altitude = geometry::Altitude;

// TWM (terrain) file format, version 3. A .twm file is a FilesContainer structured like an MWM:
// - kHeaderTag: version, coordinate bits, geometry scales with their max
//   vertical errors, the limit rect and the count of the meshes merged into the file. Parsed on
//   every registration (TwmSet::Register), so it stays small;
// - kGridTag: the lattice tables of every mesh (see MeshGrid), read once by terrain::Reader;
// - kFreqTag: the entropy coding tables of the geometry streams, one set of contexts per
//   geometry scale (see GeometryTables), read once by terrain::Reader next to the grid. One
//   table is a varuint symbols count and a (symbol, code length) varuint pair each, symbol
//   ordered: the file ships the code LENGTHS only and both sides derive the codes from them
//   by the same canonical assignment (impl::AssignCanonicalCodes). A length is capped at
//   impl::kFlatCodeBits, so a code always resolves in one flat table lookup, and the lengths
//   of a table must fill the code space exactly (a single symbol takes the length 0);
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
// MWM trg sections - extended with a per-vertex altitude channel and entropy coded. A blob is
// a plain varuint triangles count, then a bitstream of the chain events padded to a byte, so
// the per feature geometry offsets stay byte offsets. Every coded value is split into its bit
// width (the leading one bit is implicit) and the mantissa bits below it: the width is what the
// tables code, the mantissa always follows it raw.
// - a chain root is a triangle with an outer edge, coded context free (the roots are about 0.1%
//   of the geometry bytes, a table would not pay for itself): the plain lattice coordinates of
//   the first vertex, then 2 per axis deltas per the other two vertices (a root delta spans the
//   whole feature, interleaving it as below would double the longer axis), 2 raw tree bits, and
//   3 altitudes (against the feature min altitude, then the previous altitude). Every value
//   takes 5 raw bits of its width and then its mantissa;
// - every next triangle shares the (a, b) edge with its DFS parent (a, b, c) and starts with one
//   Huffman symbol of the event context - the known vertex flag over the 2 tree bits, coded
//   jointly because the pair is far from independent - followed by:
//   * a new vertex: the width tokens and mantissas of the zigzag (dcolumn, drow) residuals in
//     the dx and dy contexts, and of the zigzag altitude residual against the altitude
//     prediction alt(a) + alt(b) - alt(c) in the dz context. The vertex is predicted by the
//     parallelogram rule from the shared edge (clamped to the lattice);
//   * a known vertex: the width token and mantissa of the backreference, no coordinates and no
//     altitude. A triangulated disk has about twice as many triangles as vertices, so about a
//     half of the inner triangles close onto a vertex this geometry has already coded. Both
//     sides count the decoded vertex slots in the emission order - a chain root appends its 3
//     vertices (roots are never deduplicated, the decoder can not tell a repeated corner from a
//     new one), a new inner vertex appends 1, a known one appends nothing - and the
//     backreference is the distance from the last appended slot;
// - 2 tree bits: bit 1 - the DFS continues across the (b, c) edge, bit 2 - a subtree hangs
//   across the (c, a) edge (pushed to the stack), no bits - return to the stack top or
//   finish the chain.
// The 5 contexts of a geometry scale are Huffman coded over the whole scale: their tables live
// in the freq section, and the encoder builds them in a first walk over all the features of the
// scale, so a blob still decodes on its own. The codes are length limited to impl::kFlatCodeBits,
// which costs nothing measurable on the tiny token alphabets and lets the decoder resolve a code
// with one flat table lookup. The encode side of this codec is generator only and lives in
// topography_generator/mesh/twm_encoder.hpp (GeometryCoders, the chain walk, the package merge).

uint8_t constexpr kTwmVersion = 3;
// The cell depth of the geometry index, the same as the MWM scale index uses.
int constexpr kCellDepth = RectId::DEPTH_LEVELS;
// One .twm holds the meshes of one dynamic grid block, a handful of them in practice.
uint32_t constexpr kMaxMeshCount = 4096;

// Thrown on an unsupported .twm file: the header gates below, parsed once per registration.
// Corrupt data past the header is not defended - the files are hash checked on download - so
// the decode validates with ASSERT, and with CHECK only where a corrupt value would drive an
// allocation or a loop.
DECLARE_EXCEPTION(TwmException, RootException);

char constexpr kHeaderTag[] = HEADER_FILE_TAG;
char constexpr kGridTag[] = "grid";
char constexpr kFreqTag[] = "freq";
char constexpr kFeaturesTag[] = FEATURES_FILE_TAG;
char constexpr kIndexTag[] = "index";  /// @todo Make INDEX_FILE_TAG

inline std::string GetGeometryTag(size_t geomIndex)
{
  ASSERT_LESS(geomIndex, 10, ());
  return std::string(TRIANGLE_FILE_TAG) + static_cast<char>('0' + geomIndex);
}

struct TwmHeader
{
  struct Geometry
  {
    int8_t m_scale = 0;         // The upper zoom level served by this geometry.
    uint32_t m_maxErrorMm = 0;  // Max vertical error of the mesh, millimeters.
  };

  uint8_t m_version = kTwmVersion;
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
// The tables are built with the canonical formula - the quantized mercator point of the lattice
// node, PointDToPointU(mercator::FromLatLon(lat, lon), coordBits) - and frozen into the file, so
// the borders of the neighbor meshes and blocks - the same physical parallels and meridians -
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
  UNUSED_VALUE(limit);  // The ASSERTs below go away in Release.

  table.resize(count);
  int64_t value = ReadVarUint<uint32_t>(src);
  ASSERT_LESS(value, limit, ());
  table[0] = static_cast<uint32_t>(value);
  if (count == 1)
    return;

  int64_t delta = ReadVarUint<uint32_t>(src);
  for (size_t i = 1; i < count; ++i)
  {
    if (i > 1)
    {
      // A valid delta is under the limit, so the second difference is bounded by it too.
      int64_t const secondDelta = ReadVarInt<int64_t>(src);
      ASSERT(secondDelta >= -limit && secondDelta <= limit, (i, secondDelta));
      delta += secondDelta;
    }
    ASSERT_GREATER(delta, 0, (i));
    value += delta;
    ASSERT_LESS(value, limit, (i));
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
    ASSERT_GREATER(grid.m_samplesPerDegree, 0, ());

    uint64_t const cols = uint64_t{ReadVarUint<uint32_t>(src)} + 1;
    uint64_t const rows = uint64_t{ReadVarUint<uint32_t>(src)} + 1;
    // Every table value takes at least one byte. Always-on: the sizes drive the table
    // allocations, a corrupt pair must not reserve gigabytes.
    CHECK_LESS_OR_EQUAL(cols + rows, src.Size(), ());

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
  // Always-on: the index addresses the grids vector of the reader.
  CHECK_LESS(record.m_meshIdx, header.m_meshCount, ());

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

namespace impl
{
// The entropy coding contexts of one geometry scale, in the freq section order.
enum TokenContext : size_t
{
  kEventContext = 0,  // The inner triangle event: the known vertex flag over the 2 tree bits.
  kDxContext,         // The bit widths of the zigzag lattice column residual of a new vertex,
  kDyContext,         // of its row residual,
  kDzContext,         // of its altitude residual,
  kBackrefContext,    // and of a known vertex backreference.
  kContextCount
};

// The symbols of every context fit 5 bits: 8 event symbols, and the bit width of a value (the
// lattice residuals are bounded by the lattice, the backreferences by the feature vertices).
uint32_t constexpr kMaxToken = 31;
uint32_t constexpr kTokenAlphabet = kMaxToken + 1;
// The raw width prefix of a context free (chain root) value.
uint8_t constexpr kRootWidthBits = 5;
static_assert(kMaxToken < (1 << kRootWidthBits));
// The format limit on a token code length, and so the width of the flat decode table: every
// code resolves in a single lookup, and a file declaring a longer one is rejected on the load.
uint32_t constexpr kFlatCodeBits = 12;
// A code and the mantissa that follows it come out of one refilled window.
static_assert(kFlatCodeBits + kMaxToken - 1 <= 56);
// A flat table entry packs the code length above the symbol.
uint32_t constexpr kEntrySymbolBits = 8;
uint32_t constexpr kEntrySymbolMask = (1 << kEntrySymbolBits) - 1;
static_assert(kMaxToken <= kEntrySymbolMask);
// The known vertex flag of an event symbol, above the 2 tree bits.
uint32_t constexpr kKnownVertexToken = 4;
uint32_t constexpr kMaxEventToken = 7;

// The bit input of a geometry blob. The wire is the least significant bit first one
// coding::BitWriter lays out and coding::BitReader walks - the first written bit is the lowest
// bit of the first byte - but this is a sibling of the latter rather than a replacement: it
// peeks without consuming (the flat token tables index a fixed width prefix of the stream),
// keeps a 64 bit window over a locally buffered chunk instead of a virtual reader call per
// byte, and zero pads past the blob end so a fixed width peek never fails.
template <typename Source>
class BlobBitReader
{
public:
  explicit BlobBitReader(Source & src) : m_src(src), m_bytesLeft(src.Size()) {}

  // Tops the window up to at least 56 bits, or to everything the blob has left: one unaligned
  // load and three arithmetic ops, with the chunk boundary and the blob tail out of line.
  void Refill()
  {
    // Both loads shift by m_bitsInWindow, so it must stay a defined shift count: the fast path
    // below leaves it at 56..63 and the tail caps it at 63.
    ASSERT_LESS(m_bitsInWindow, 64, ());
    if (m_pos + kWordBytes > m_len)
      return SlowRefill();

    uint64_t next;
    std::memcpy(&next, m_buf + m_pos, sizeof(next));
    // The window bits above m_bitsInWindow are either zero or the very bits this load repeats
    // (m_pos stays on the byte the window ends inside), so the OR never corrupts them.
    m_window |= SwapIfBigEndianMacroBased(next) << m_bitsInWindow;
    m_pos += (63 - m_bitsInWindow) >> 3;
    m_bitsInWindow |= 56;
  }

  // The window a Refill() has just topped up: the next bits of the blob, least significant
  // first, zero padded past its end. A code and its mantissa are read out of one window.
  uint64_t Window() const { return m_window; }

  // Zero padded past the end of the blob, so a peek of a fixed width - a flat table lookup
  // always takes one - never fails.
  uint32_t PeekBits(uint32_t n)
  {
    ASSERT_LESS_OR_EQUAL(n, 32, ());
    Refill();
    return static_cast<uint32_t>(m_window & bits::GetFullMask(static_cast<uint8_t>(n)));
  }

  // Consuming what the blob does not have is the error a peek is not. Every skip follows a
  // peek or a refill of this read, so the window is already topped up and a shortfall means
  // the stream is truncated.
  void SkipPeekedBits(uint32_t n)
  {
    ASSERT_LESS_OR_EQUAL(n, m_bitsInWindow, ("Truncated geometry bitstream"));
    m_window >>= n;
    m_bitsInWindow -= n;
  }

  uint32_t ReadBits(uint32_t n)
  {
    uint32_t const bits = PeekBits(n);
    SkipPeekedBits(n);
    return bits;
  }

private:
  // The chunk is filled in one source call and the fast refill covers all but its last 7
  // bytes, so this runs about once per 250 bytes of a blob.
  void SlowRefill()
  {
    // Fewer than kWordBytes are buffered: compact them to the front and top the chunk up.
    uint32_t const left = m_len - m_pos;
    std::memmove(m_buf, m_buf + m_pos, left);
    m_pos = 0;
    m_len = left;
    if (m_bytesLeft != 0)
    {
      auto const want = static_cast<uint32_t>(std::min<uint64_t>(kChunkSize - left, m_bytesLeft));
      m_src.Read(m_buf + m_len, want);
      m_bytesLeft -= want;
      m_len += want;
    }
    // The slack past the data is the zero padding a peek past the blob end must return.
    std::memset(m_buf + m_len, 0, kWordBytes);
    if (m_len >= kWordBytes)
      return Refill();

    // The blob tail: the same load over the zero padding, counting in the buffered bytes only
    // so that the window never claims bits the blob does not have. The count stops at 63 bits,
    // which keeps the shift of the next refill defined and is far above the 42 bits a token
    // and its mantissa take together; the bytes left over stay in the buffer.
    uint64_t next;
    std::memcpy(&next, m_buf, sizeof(next));
    m_window |= SwapIfBigEndianMacroBased(next) << m_bitsInWindow;
    uint32_t const bytes = std::min(m_len, (63 - m_bitsInWindow) / 8);
    m_pos = bytes;
    m_bitsInWindow += 8 * bytes;
  }

  static uint32_t constexpr kChunkSize = 256;
  static uint32_t constexpr kWordBytes = 8;

  Source & m_src;
  uint64_t m_window = 0;
  uint32_t m_bitsInWindow = 0;
  // The blob bytes the source still holds: the chunk refill needs the count to size its read,
  // and Source::Size() is a virtual call down the reader chain.
  uint64_t m_bytesLeft = 0;
  uint32_t m_pos = 0;
  uint32_t m_len = 0;
  // kWordBytes of slack keep the zero padded tail load in bounds.
  uint8_t m_buf[kChunkSize + kWordBytes];
};

// The bits a value of the given width takes below its implicit leading one. Both feeders are
// bounded - a raw root width is 5 bits and a table symbol is validated at the load - so a
// mantissa is under kMaxToken bits and always fits the window next to its width token.
inline uint32_t MantissaBits(uint32_t width)
{
  ASSERT_LESS_OR_EQUAL(width, kMaxToken, ());
  return width == 0 ? 0 : width - 1;
}

// Returns the whole value back from the bits below its implicit leading one, which |below|
// holds at its low end: the width spells the rest of it, and a zero width is the value zero.
inline uint32_t ValueFromWidth(uint32_t width, uint64_t below)
{
  if (width == 0)
    return 0;
  return (uint32_t{1} << (width - 1)) |
         static_cast<uint32_t>(below & bits::GetFullMask(static_cast<uint8_t>(width - 1)));
}

// The width and the mantissa come out of one window: 5 + 30 bits at the very most.
static_assert(kRootWidthBits + kMaxToken - 1 <= 56);

template <typename Source>
uint32_t ReadRootValue(BlobBitReader<Source> & in)
{
  in.Refill();
  uint64_t const window = in.Window();
  auto const width = static_cast<uint32_t>(window & bits::GetFullMask(kRootWidthBits));
  in.SkipPeekedBits(kRootWidthBits + MantissaBits(width));
  return ValueFromWidth(width, window >> kRootWidthBits);
}

// The wire code of one token: the bits the decoder consumes least significant first, and how
// many of them. A context that coded a single token spends no bits at all on it.
struct TokenCode
{
  uint32_t m_bits = 0;
  uint8_t m_length = 0;
};

// The low |length| bits of |code|, in the reverse order.
inline uint32_t ReverseBits(uint32_t code, uint32_t length)
{
  uint32_t reversed = 0;
  for (uint32_t i = 0; i < length; ++i, code >>= 1)
    reversed = (reversed << 1) | (code & 1);
  return reversed;
}

// The canonical code of the given lengths, in the wire bit order. The freq section ships the
// lengths only and BOTH sides run this, so the codes never travel and can not disagree.
// A canonical code is laid out most significant bit first - that is what makes it prefix free
// by construction - while the decoder consumes a code least significant bit first, so every
// code is reversed within its length (the DEFLATE convention): the reversal maps a shared
// leading prefix onto a shared trailing one, keeping both the prefix property and the flat
// table indexing. A zero length (the single token of a one token context, and every token the
// context does not code) takes no bits.
inline void AssignCanonicalCodes(std::array<uint8_t, kTokenAlphabet> const & lengths,
                                 std::array<TokenCode, kTokenAlphabet> & codes)
{
  std::array<uint32_t, kFlatCodeBits + 1> perLength = {};
  for (uint8_t const length : lengths)
    if (length != 0)
      ++perLength[length];

  std::array<uint32_t, kFlatCodeBits + 1> nextCode = {};
  uint32_t code = 0;
  for (uint32_t length = 1; length <= kFlatCodeBits; ++length)
  {
    code = (code + perLength[length - 1]) << 1;
    nextCode[length] = code;
  }

  for (uint32_t symbol = 0; symbol < kTokenAlphabet; ++symbol)
  {
    uint32_t const length = lengths[symbol];
    codes[symbol].m_length = static_cast<uint8_t>(length);
    codes[symbol].m_bits = length == 0 ? 0 : ReverseBits(nextCode[length]++, length);
  }
}

// The decode side of one context: the flat table of its freq section table. The file ships the
// code lengths only, both sides run the same canonical assignment over them, and the lengths
// are validated to fill the code space exactly - so every table slot resolves a code and a
// read is one lookup, with no walk and no unassigned slot to check for.
class TokenTable
{
public:
  // Parses one freq section table: the count of the coded symbols, then a (symbol, code
  // length) varuint pair each.
  template <typename Source>
  void Deserialize(Source & src)
  {
    uint64_t const count = ReadVarUint<uint64_t>(src);
    // Always-on: the count drives the parse loop below.
    CHECK_LESS_OR_EQUAL(count, kTokenAlphabet, ());
    if (count == 0)
    {
      // A context the scale never coded. One poisoned entry keeps a read from it on the code
      // length ASSERT its caller already makes, instead of an emptiness branch per read.
      m_tableBits = 0;
      m_tableMask = 0;
      m_table.assign(1, kPoisonEntry);
      return;
    }

    std::array<uint8_t, kTokenAlphabet> lengths = {};
    uint32_t coded = 0;
    uint64_t kraft = 0;
    for (uint64_t i = 0; i < count; ++i)
    {
      uint64_t const symbol = ReadVarUint<uint64_t>(src);
      uint64_t const length = ReadVarUint<uint64_t>(src);
      // Always-on: the symbol indexes the lengths array, the length feeds the shift below.
      CHECK(symbol <= kMaxToken && length <= kFlatCodeBits, (symbol, length));
      uint32_t const bit = uint32_t{1} << symbol;
      ASSERT(!(coded & bit), ("A duplicate token table symbol", symbol));
      coded |= bit;
      lengths[symbol] = static_cast<uint8_t>(length);
      kraft += uint64_t{1} << (kFlatCodeBits - length);
    }
    // The lengths must fill the code space exactly: a shorter sum leaves table slots no code
    // reaches, a longer one is not a prefix code at all. The single symbol of a one symbol
    // context has the length 0 and fills the space on its own.
    ASSERT_EQUAL(kraft, uint64_t{1} << kFlatCodeBits, ());
    UNUSED_VALUE(kraft);

    std::array<TokenCode, kTokenAlphabet> codes;
    AssignCanonicalCodes(lengths, codes);
    BuildFlatTable(lengths, codes, coded);
  }

  // The packed (code length, symbol) of the code the window starts with. The length of the
  // poisoned table above is over kFlatCodeBits, which the readers ASSERT against on their way
  // to the bit count they consume: a valid stream never reads from an absent context.
  uint32_t Lookup(uint64_t window) const { return m_table[window & m_tableMask]; }

private:
  static uint32_t constexpr kPoisonLength = kFlatCodeBits + 1;
  static uint16_t constexpr kPoisonEntry = static_cast<uint16_t>(kPoisonLength << kEntrySymbolBits);

  // Every code of a complete prefix code fills the table slots whose low bits are the code
  // itself: the reader peeks a fixed width and consumes only the length the slot declares.
  void BuildFlatTable(std::array<uint8_t, kTokenAlphabet> const & lengths,
                      std::array<TokenCode, kTokenAlphabet> const & codes, uint32_t coded)
  {
    uint32_t maxLength = 0;
    for (uint8_t const length : lengths)
      maxLength = std::max(maxLength, uint32_t{length});

    m_tableBits = maxLength;
    m_tableMask = static_cast<size_t>(bits::GetFullMask(static_cast<uint8_t>(m_tableBits)));
    m_table.assign(size_t{1} << m_tableBits, 0);
    for (uint32_t symbol = 0; symbol < kTokenAlphabet; ++symbol)
    {
      if ((coded & (uint32_t{1} << symbol)) == 0)
        continue;
      uint32_t const length = lengths[symbol];
      auto const packed = static_cast<uint16_t>((length << kEntrySymbolBits) | symbol);
      uint32_t const fills = uint32_t{1} << (m_tableBits - length);
      for (uint32_t fill = 0; fill < fills; ++fill)
        m_table[(fill << length) | codes[symbol].m_bits] = packed;
    }
  }

  // The flat table of the m_tableBits wide code prefixes. A table that was never deserialized
  // is poisoned like an absent context, so a read from it asserts instead of indexing nothing.
  std::vector<uint16_t> m_table = {kPoisonEntry};
  size_t m_tableMask = 0;
  uint32_t m_tableBits = 0;
};
}  // namespace impl

// The entropy tables of one geometry scale (one freq section entry), decode side.
class GeometryTables
{
public:
  template <typename Source>
  void Deserialize(Source & src)
  {
    for (auto & context : m_contexts)
      context.Deserialize(src);
  }

  // One token: one refill, one table lookup. A code takes at most impl::kFlatCodeBits bits,
  // so the refilled window always holds it whole.
  template <typename Source>
  uint32_t ReadToken(impl::BlobBitReader<Source> & in, impl::TokenContext ctx) const
  {
    in.Refill();
    uint32_t const entry = m_contexts[ctx].Lookup(in.Window());
    uint32_t const length = entry >> impl::kEntrySymbolBits;
    ASSERT_LESS_OR_EQUAL(length, impl::kFlatCodeBits, ("A token of a context the file has no table for"));
    in.SkipPeekedBits(length);
    return entry & impl::kEntrySymbolMask;
  }

  // A value: the token of its bit width, then the mantissa bits below the implicit leading
  // one. Both come out of the SAME refilled window - a code is at most 12 bits and a mantissa
  // at most 30 - so a value costs one refill, one table load and a couple of shifts.
  template <typename Source>
  uint32_t ReadValue(impl::BlobBitReader<Source> & in, impl::TokenContext ctx) const
  {
    in.Refill();
    uint64_t const window = in.Window();
    uint32_t const entry = m_contexts[ctx].Lookup(window);
    uint32_t const length = entry >> impl::kEntrySymbolBits;
    ASSERT_LESS_OR_EQUAL(length, impl::kFlatCodeBits, ("A token of a context the file has no table for"));
    uint32_t const width = entry & impl::kEntrySymbolMask;
    in.SkipPeekedBits(length + impl::MantissaBits(width));
    return impl::ValueFromWidth(width, window >> length);
  }

private:
  std::array<impl::TokenTable, impl::kContextCount> m_contexts;
};

// The freq section: the token tables of every geometry scale, in the scale order, 5 contexts
// each (see impl::TokenContext). The encoder appends the tables of a scale right after it has
// walked the features of that scale, see GeometryCoders in twm_encoder.hpp.
template <typename Source>
void DeserializeFreqs(Source & src, TwmHeader const & header, std::vector<GeometryTables> & tables)
{
  tables.clear();
  tables.resize(header.m_geometries.size());
  for (auto & scale : tables)
    scale.Deserialize(src);
}

// The working vectors of the geometry decoder. They are per feature, but one tile read decodes
// hundreds of features, so the caller owns them for the whole read: the decoder clears them and
// never shrinks them, and the pages of a tile are first touched once instead of once per feature.
struct GeometryScratch
{
  // The DFS stack entry: the edge shared with the parent triangle and the parent vertex
  // opposite it (the parallelogram prediction base), as slots of the emission list.
  struct StackEntry
  {
    uint32_t m_u, m_v, m_pred;
  };

  void Clear()
  {
    m_lattice.clear();
    m_altitudes.clear();
    m_meshIndex.clear();
    m_stack.clear();
  }

  // The emitted vertex slots: the feature-local prediction sources, NOT read back from the
  // mesh (the altitude deltas were encoded against this feature's own values - a shared
  // border vertex may keep another block's altitude in the mesh - and the point prediction
  // needs the lattice coordinates the mesh does not store), and the list the known vertex
  // backreferences address. A vertex the encoder emitted twice (chains sharing a corner)
  // takes two slots with identical values - the mesh index is the same for both.
  std::vector<m2::PointU> m_lattice;
  std::vector<int32_t> m_altitudes;
  std::vector<uint32_t> m_meshIndex;
  std::vector<StackEntry> m_stack;
};

namespace impl
{
// The parallelogram prediction of coding::PredictPointInTriangle in the lattice ints:
// the new vertex of the (u, v) edge continues the (pred, u, v) triangle. Clamping keeps
// the prediction a valid lattice pair and the deltas bounded by the lattice size (which
// is under 2^30 - the table values are strictly increasing and fit the coordinate bits -
// so the zigzag residuals of an inner event always fit the kMaxToken bit width).
inline m2::PointU PredictLattice(m2::PointU const & u, m2::PointU const & v, m2::PointU const & pred,
                                 m2::PointU const & maxLattice)
{
  auto const predict = [](int64_t value, uint32_t maxValue)
  { return static_cast<uint32_t>(std::clamp<int64_t>(value, 0, maxValue)); };
  return {predict(int64_t{u.x} + v.x - pred.x, maxLattice.x), predict(int64_t{u.y} + v.y - pred.y, maxLattice.y)};
}

// The decoded lattice pair is a pair of table indices: a valid stream never fails here, the
// roots are coded plainly and the inner vertices land exactly on the encoder ones, inside
// the lattice.
inline m2::PointU CheckLattice(int64_t col, int64_t row, m2::PointU const & maxLattice)
{
  ASSERT(col >= 0 && col <= maxLattice.x && row >= 0 && row <= maxLattice.y, (col, row, maxLattice));
  return {static_cast<uint32_t>(col), static_cast<uint32_t>(row)};
}

// Appends the decoded feature chains straight into the tile mesh: the vertices dedup
// globally there (one map per tile, no per-feature maps or intermediate vectors). The
// chain walk state stays feature-local: the codec point and altitude predictions must
// use this feature's OWN decoded values, which may differ from the mesh ones on a
// border vertex shared with an already added block of another data version.
template <typename Source>
void DecodeChains(Source & src, MeshGrid const & grid, GeometryTables const & tables, Altitude minAltitude,
                  GeometryScratch & scratch, TileMesh & mesh)
{
  uint64_t const count = ReadVarUint<uint64_t>(src);
  // The bitstream can not hold more events than it has bits. Always-on: a corrupt count must
  // not drive the reserves below and the decode loop.
  CHECK_LESS_OR_EQUAL(count, 8 * src.Size(), ());

  BlobBitReader<Source> in(src);
  m2::PointU const maxLattice = grid.GetMax();

  scratch.Clear();
  auto & lattice = scratch.m_lattice;
  auto & altitudes = scratch.m_altitudes;
  auto & meshIndex = scratch.m_meshIndex;
  auto & stack = scratch.m_stack;

  // The writer splits a geometry at a few thousand triangles, so the cap never bites a valid
  // blob - it only keeps a count that passed the bitstream bound above from reserving
  // gigabytes before the decode loop rejects the stream.
  size_t constexpr kMaxReserveTriangles = 1 << 16;
  size_t const reserved = std::min<size_t>(count, kMaxReserveTriangles);
  // A closed mesh chunk holds about twice as many triangles as vertices (T ~ 2V), and a blob
  // of |count| triangles adds count + 2 vertices at the very most: reserve the typical half.
  // The over reserve of a sparse feature is transient, the tile mesh lives for one tile read.
  mesh.ReserveAdditional(reserved / 2 + 3, reserved);
  // A chain root takes 3 slots for its triangle and every next event at most one, so a single
  // chain fits count + 2 slots. The scratch keeps whatever the largest feature needed, so
  // these reserves are no-ops after the first few features of a tile.
  lattice.reserve(reserved + 2);
  altitudes.reserve(reserved + 2);
  meshIndex.reserve(reserved + 2);
  // The DFS stack holds the pending branches of one chain: shallow in practice.
  size_t constexpr kStackReserve = 256;
  stack.reserve(kStackReserve);

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

  using Ctx = GeometryScratch::StackEntry;

  size_t decoded = 0;
  while (decoded < count)
  {
    // A new chain root, per axis deltas (the reads are ordered, not function arguments).
    uint32_t const col0 = ReadRootValue(in);
    uint32_t const row0 = ReadRootValue(in);
    m2::PointU const p0 = CheckLattice(col0, row0, maxLattice);
    int32_t const dcol1 = bits::ZigZagDecode(ReadRootValue(in));
    int32_t const drow1 = bits::ZigZagDecode(ReadRootValue(in));
    m2::PointU const p1 = CheckLattice(int64_t{p0.x} + dcol1, int64_t{p0.y} + drow1, maxLattice);
    int32_t const dcol2 = bits::ZigZagDecode(ReadRootValue(in));
    int32_t const drow2 = bits::ZigZagDecode(ReadRootValue(in));
    m2::PointU const p2 = CheckLattice(int64_t{p1.x} + dcol2, int64_t{p1.y} + drow2, maxLattice);
    uint8_t treeBits = static_cast<uint8_t>(in.ReadBits(2));
    int64_t const z0 = minAltitude + int64_t{ReadRootValue(in)};
    int64_t const z1 = z0 + bits::ZigZagDecode(ReadRootValue(in));
    int64_t const z2 = z1 + bits::ZigZagDecode(ReadRootValue(in));

    uint32_t pa = addVertex(p0, z0), pb = addVertex(p1, z1), pc = addVertex(p2, z2);
    emitTriangle(pa, pb, pc);
    ++decoded;

    ASSERT(stack.empty(), ());
    while (true)
    {
      Ctx cur;
      if (treeBits & 2)
        stack.emplace_back(pa, pc, pb);
      if (treeBits & 1)
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

      uint32_t const event = tables.ReadToken(in, kEventContext);
      ASSERT_LESS_OR_EQUAL(event, kMaxEventToken, ());
      treeBits = event & 3;
      uint32_t w;
      if (event & kKnownVertexToken)
      {
        // A vertex this geometry has already decoded: it takes a slot of the emission list,
        // with its lattice pair, altitude and mesh index, and appends no new one.
        uint32_t const backref = tables.ReadValue(in, kBackrefContext);
        ASSERT_LESS(backref, lattice.size(), ());
        w = static_cast<uint32_t>(lattice.size() - 1 - backref);
      }
      else
      {
        uint32_t const zigzagCol = tables.ReadValue(in, kDxContext);
        uint32_t const zigzagRow = tables.ReadValue(in, kDyContext);
        uint32_t const zigzagAlt = tables.ReadValue(in, kDzContext);
        auto const prediction = PredictLattice(lattice[cur.m_u], lattice[cur.m_v], lattice[cur.m_pred], maxLattice);
        m2::PointU const pw = CheckLattice(int64_t{prediction.x} + bits::ZigZagDecode(zigzagCol),
                                           int64_t{prediction.y} + bits::ZigZagDecode(zigzagRow), maxLattice);
        int64_t const zw =
            int64_t{altitudes[cur.m_u]} + altitudes[cur.m_v] - altitudes[cur.m_pred] + bits::ZigZagDecode(zigzagAlt);
        w = addVertex(pw, zw);
      }
      emitTriangle(cur.m_u, cur.m_v, w);
      ++decoded;
      // Always-on: a crafted zero-length-code stream would otherwise consume no bits
      // and loop this walk forever - an OOM hang instead of the accepted crash.
      CHECK_LESS_OR_EQUAL(decoded, count, ());
      pa = cur.m_u;
      pb = cur.m_v;
      pc = w;
    }
  }
}
}  // namespace impl

// Decodes one geometry located by a FeatureRecord into the tile mesh, with the tables of its
// scale. The record supplies the altitudes base; the grid of the record mesh maps the lattice
// coordinates.
template <typename Source>
void DeserializeFeatureGeometry(Source & src, MeshGrid const & grid, GeometryTables const & tables,
                                FeatureRecord const & record, GeometryScratch & scratch, TileMesh & mesh)
{
  impl::DecodeChains(src, grid, tables, record.m_minAltitude, scratch, mesh);
}

}  // namespace terrain
