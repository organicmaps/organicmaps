#pragma once

#include "indexer/cell_coverer.hpp"
#include "indexer/cell_id.hpp"
#include "indexer/cell_value_pair.hpp"
#include "indexer/interval_index_builder.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include "defines.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <vector>

namespace terrain
{
using Altitude = geometry::Altitude;

// TWM (terrain) file format, version 1. A .twm file is a FilesContainer structured like an MWM:
// - kHeaderTag: version, coordinate bits, geometry scales with their max vertical errors,
//   and the limit rect. The limit rect center is the base point for all point deltas;
// - kFeaturesTag: FeatureRecords back to back, ordered by the feature center cell id
//   (nearby features lay nearby in the file, like MWM features). A record stores the
//   altitudes range, the feature limit rect and one offset per geometry scale;
// - kGeometryTags[g] ("trg0", "trg1", ...): triangle geometries of one scale, separated
//   per section (like MWM trgN), so rendering at one scale keeps a dense FilesContainer
//   cache instead of skipping the other scale blobs;
// - kIndexTag: an interval index over the RectId cells covering the feature rects,
//   mapping cell intervals to the FeatureRecord offsets (the MWM scale index scheme).
//
// A TrianglesFeature geometry is a chunk of the terrain mesh serialized as DFS tree chains
// with the parallelogram point prediction - the serial::TrianglesChainSaver scheme of the
// MWM trg sections - extended with a per-vertex altitude channel:
// - a chain root is a triangle with an outer edge: 3 point deltas (against the TWM center,
//   then the previous point) and 3 altitudes (against the feature min altitude, then the
//   previous altitude);
// - every next triangle shares the (a, b) edge with its DFS parent (a, b, c) and takes one
//   varint (pointDelta << 2 | treeBits), where the new vertex is predicted by the
//   parallelogram rule from the shared edge, plus one varint altitude delta against the
//   altitude parallelogram prediction alt(a) + alt(b) - alt(c);
// - 2 tree bits: bit 1 - the DFS continues across the (b, c) edge, bit 2 - a subtree hangs
//   across the (c, a) edge (pushed to the stack), no bits - return to the stack top or
//   finish the chain.
//
// All vertices are quantized to m2::PointU against the global mercator bounds, and their
// geographic positions are computed from global integer arc-second coordinates,
// so a vertex shared by two features or two TWM
// files decodes to the identical point - adjacency can be restored by hashing points.

uint8_t constexpr kTwmVersion = 1;
// ~0.3 m of a horizontal precision, enough against the ~30 m DEM samples step.
uint8_t constexpr kTerrainCoordBits = 27;
// The cell depth of the geometry index, the same as the MWM scale index uses.
int constexpr kCellDepth = RectId::DEPTH_LEVELS;
size_t constexpr kMaxCellsPerFeature = 32;

// Thrown on corrupt or unsupported .twm data. The decoding side must stay catchable
// on the client, unlike the always-aborting CHECKs of the encoding side.
DECLARE_EXCEPTION(TwmException, RootException);

char constexpr kHeaderTag[] = HEADER_FILE_TAG;
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

  size_t GetGeometryIndex(int scale) const
  {
    ASSERT(!m_geometries.empty(), ());
    for (size_t i = 0; i + 1 < m_geometries.size(); ++i)
      if (scale <= m_geometries[i].m_scale)
        return i;
    return m_geometries.size() - 1;
  }

  m2::PointU GetCenter() const
  {
    return {m_limitLB.x + (m_limitRT.x - m_limitLB.x) / 2, m_limitLB.y + (m_limitRT.y - m_limitLB.y) / 2};
  }

  m2::RectD GetLimitRect() const
  {
    return {PointUToPointD(m_limitLB, m_coordBits), PointUToPointD(m_limitRT, m_coordBits)};
  }

  // The base point of all the point deltas in the file, like the MWM-wide base point.
  serial::GeometryCodingParams GetCodingParams() const
  {
    return {m_coordBits, PointUToPointD(GetCenter(), m_coordBits)};
  }

  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK(m_limitLB.x <= m_limitRT.x && m_limitLB.y <= m_limitRT.y, ());
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
  }
};

// The FeatureRecord of the features section: everything needed to cull a feature and locate
// its geometries. The geometry offsets are absolute (the records are accessed randomly via
// the index).
struct FeatureRecord
{
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
  CHECK(record.m_rectLB.x >= header.m_limitLB.x && record.m_rectLB.y >= header.m_limitLB.y &&
            record.m_rectRT.x <= header.m_limitRT.x && record.m_rectRT.y <= header.m_limitRT.y,
        ());
  CHECK(record.m_rectLB.x <= record.m_rectRT.x && record.m_rectLB.y <= record.m_rectRT.y, ());
  CHECK_LESS_OR_EQUAL(record.m_minAltitude, record.m_maxAltitude, ());

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

// Encoder input: quantized vertices deduplicated within the feature and CCW vertex index
// triples per geometry scale.
struct FeatureData
{
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

// Decoded TrianglesFeature: vertices with altitudes deduplicated within the feature and
// CCW vertex index triples of the requested geometry scale.
struct Triangles
{
  std::vector<m2::PointD> m_points;
  std::vector<Altitude> m_altitudes;
  std::vector<uint32_t> m_triangles;
  m2::RectD m_rect;
  Altitude m_minAltitude = 0;
  Altitude m_maxAltitude = 0;
  // DFS chains of the decoded geometry, mostly 1 (a connected mesh chunk).
  uint32_t m_chainsCount = 0;
};

namespace impl
{
uint32_t constexpr kNoTri = std::numeric_limits<uint32_t>::max();

inline uint64_t EdgeKey(uint32_t u, uint32_t v)
{
  return (static_cast<uint64_t>(u) << 32) | v;
}

inline uint64_t PointKey(m2::PointU const & pt)
{
  return (static_cast<uint64_t>(pt.x) << 32) | pt.y;
}

// DFS tree chains encoder, the scheme of serial::TrianglesChainSaver extended with altitudes.
class ChainEncoder
{
public:
  ChainEncoder(FeatureData const & data, std::vector<uint32_t> const & triangles,
               serial::GeometryCodingParams const & cp, Altitude minAltitude)
    : m_data(data)
    , m_triangles(triangles)
    , m_basePoint(cp.GetBasePoint())
    , m_maxPoint(serial::pts::GetMaxPoint(cp))
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
  m2::PointU const & Point(uint32_t v) const { return m_data.m_points[v]; }
  int32_t Alt(uint32_t v) const { return m_data.m_altitudes[v]; }

  uint32_t Neighbor(uint32_t u, uint32_t v) const
  {
    auto const it = m_edgeToTri.find(EdgeKey(v, u));
    return it == m_edgeToTri.end() ? kNoTri : it->second;
  }

  template <typename Sink>
  void WritePacked(Sink & sink, uint64_t delta, uint8_t bits)
  {
    CHECK_EQUAL(delta >> 62, 0, ());
    WriteVarUint(sink, (delta << 2) | bits);
  }

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
    WriteVarUint(sink, coding::EncodePointDeltaAsUint(Point(a), m_basePoint));
    WriteVarUint(sink, coding::EncodePointDeltaAsUint(Point(b), Point(a)));
    WritePacked(sink, coding::EncodePointDeltaAsUint(Point(c), Point(b)), bits);
    WriteVarUint(sink, static_cast<uint32_t>(Alt(a) - m_minAltitude));
    WriteVarInt(sink, Alt(b) - Alt(a));
    WriteVarInt(sink, Alt(c) - Alt(b));

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
      auto const prediction =
          coding::PredictPointInTriangle(m_maxPoint, Point(cur.m_u), Point(cur.m_v), Point(cur.m_pred));
      WritePacked(sink, coding::EncodePointDeltaAsUint(Point(w), prediction), bits);
      WriteVarInt(sink, Alt(w) - (Alt(cur.m_u) + Alt(cur.m_v) - Alt(cur.m_pred)));
      ++emitted;
    }
    return emitted;
  }

  FeatureData const & m_data;
  std::vector<uint32_t> const & m_triangles;
  m2::PointU const m_basePoint;
  m2::PointD const m_maxPoint;
  int32_t const m_minAltitude;
  std::unordered_map<uint64_t, uint32_t> m_edgeToTri;
  std::vector<bool> m_visited;
};

template <typename Source>
void DecodeChains(Source & src, serial::GeometryCodingParams const & cp, Altitude minAltitude, Triangles & out)
{
  uint64_t const count = ReadVarUint<uint64_t>(src);
  // Every encoded triangle takes at least 2 bytes, don't let a corrupt count drive
  // a huge allocation which would bypass the corrupt data handling.
  if (count > src.Size())
    MYTHROW(TwmException, ("Corrupt triangles count", count));
  out.m_points.clear();
  out.m_altitudes.clear();
  out.m_triangles.clear();
  // The count upper bound above is loose (the source spans to the section end),
  // don't let a corrupt value drive a huge upfront allocation.
  out.m_triangles.reserve(std::min(count, uint64_t{1} << 20) * 3);
  out.m_chainsCount = 0;

  m2::PointU const basePoint = cp.GetBasePoint();
  m2::PointD const maxPoint(serial::pts::GetMaxPoint(cp));
  std::vector<m2::PointU> points;  // Quantized vertices, parallel to out.m_points.
  std::unordered_map<uint64_t, uint32_t> pointToIndex;

  auto const addVertex = [&](m2::PointU const & pt, int64_t alt) -> uint32_t
  {
    auto const [it, inserted] = pointToIndex.emplace(PointKey(pt), static_cast<uint32_t>(points.size()));
    if (inserted)
    {
      points.push_back(pt);
      out.m_points.push_back(PointUToPointD(pt, cp.GetCoordBits()));
      out.m_altitudes.push_back(static_cast<Altitude>(alt));
    }
    else
      ASSERT_EQUAL(out.m_altitudes[it->second], alt, ());
    return it->second;
  };

  // Not named "emit" to keep the header compatible with the Qt keyword macros.
  auto const emitTriangle = [&](uint32_t a, uint32_t b, uint32_t c)
  { out.m_triangles.insert(out.m_triangles.end(), {a, b, c}); };

  struct Ctx
  {
    uint32_t m_u, m_v, m_pred;
  };
  std::vector<Ctx> stack;

  size_t decoded = 0;
  while (decoded < count)
  {
    // A new chain root.
    ++out.m_chainsCount;
    m2::PointU const p0 = coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(src), basePoint);
    m2::PointU const p1 = coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(src), p0);
    uint64_t const packed = ReadVarUint<uint64_t>(src);
    uint8_t bits = packed & 3;
    m2::PointU const p2 = coding::DecodePointDeltaFromUint(packed >> 2, p1);
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
        stack.push_back({pa, pc, pb});
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
      bits = next & 3;
      auto const prediction =
          coding::PredictPointInTriangle(maxPoint, points[cur.m_u], points[cur.m_v], points[cur.m_pred]);
      m2::PointU const pw = coding::DecodePointDeltaFromUint(next >> 2, prediction);
      int64_t const zw = int64_t{out.m_altitudes[cur.m_u]} + out.m_altitudes[cur.m_v] - out.m_altitudes[cur.m_pred] +
                         ReadVarInt<int32_t>(src);

      uint32_t const w = addVertex(pw, zw);
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
                              TwmHeader const & header, Altitude minAltitude)
{
  CHECK(!data.m_points.empty(), ());
  CHECK_EQUAL(data.m_points.size(), data.m_altitudes.size(), ());

  impl::ChainEncoder encoder(data, triangles, header.GetCodingParams(), minAltitude);
  encoder.Save(sink);
}

// Decodes one geometry located by a FeatureRecord. The record supplies the altitudes range
// and the rect; the header supplies the shared coding params.
template <typename Source>
void DeserializeFeatureGeometry(Source & src, TwmHeader const & header, FeatureRecord const & record, Triangles & out)
{
  out.m_minAltitude = record.m_minAltitude;
  out.m_maxAltitude = record.m_maxAltitude;
  out.m_rect = record.GetRect(header.m_coordBits);
  impl::DecodeChains(src, header.GetCodingParams(), record.m_minAltitude, out);
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
