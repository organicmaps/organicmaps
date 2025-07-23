#pragma once

#include "geometry/point2d.hpp"

#include "coding/point_coding.hpp"
#include "coding/tesselator_decl.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/array_adapters.hpp"
#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <functional>
#include <list>
#include <vector>

namespace coding
{
using InPointsT = array_read<m2::PointU>;
using InDeltasT = array_read<uint64_t>;
using OutPointsT = array_write<m2::PointU>;
using OutDeltasT = array_write<uint64_t>;

// Stores the difference of two points to a single unsigned 64-bit integer.
// It is not recommended to use this function: consider EncodePointDelta instead.
uint64_t EncodePointDeltaAsUint(m2::PointU const & actual, m2::PointU const & prediction);

m2::PointU DecodePointDeltaFromUint(uint64_t delta, m2::PointU const & prediction);

// Writes the difference of two 2d vectors to sink.
template <typename Sink>
void EncodePointDelta(Sink & sink, m2::PointU const & curr, m2::PointU const & next)
{
  auto const dx = base::asserted_cast<int32_t>(next.x) - base::asserted_cast<int32_t>(curr.x);
  auto const dy = base::asserted_cast<int32_t>(next.y) - base::asserted_cast<int32_t>(curr.y);
  WriteVarInt(sink, dx);
  WriteVarInt(sink, dy);
}

// Reads the encoded difference from |source| and returns the
// point equal to |base| + difference.
template <typename Source>
m2::PointU DecodePointDelta(Source & source, m2::PointU const & base)
{
  auto const dx = ReadVarInt<int32_t>(source);
  auto const dy = ReadVarInt<int32_t>(source);
  ASSERT(int(base.x) + dx >= 0 && int(base.y) + dy >= 0, (base, dx, dy));
  return m2::PointU(base.x + dx, base.y + dy);
}

/// Predict next point for polyline with given previous points (p1, p2).
m2::PointU PredictPointInPolyline(m2::PointD const & maxPoint, m2::PointU const & p1, m2::PointU const & p2);

/// Predict next point for polyline with given previous points (p1, p2, p3).
m2::PointU PredictPointInPolyline(m2::PointD const & maxPoint, m2::PointU const & p1, m2::PointU const & p2,
                                  m2::PointU const & p3);

/// Predict point for neighbour triangle with given
/// previous triangle (p1, p2, p3) and common edge (p1, p2).
m2::PointU PredictPointInTriangle(m2::PointD const & maxPoint, m2::PointU const & p1, m2::PointU const & p2,
                                  m2::PointU const & p3);

void EncodePolylinePrev1(InPointsT const & points, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodePolylinePrev1(InDeltasT const & deltas, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolylinePrev2(InPointsT const & points, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodePolylinePrev2(InDeltasT const & deltas, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolylinePrev3(InPointsT const & points, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodePolylinePrev3(InDeltasT const & deltas, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolyline(InPointsT const & points, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                    OutDeltasT & deltas);

void DecodePolyline(InDeltasT const & deltas, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                    OutPointsT & points);

void EncodeTriangleStrip(InPointsT const & points, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodeTriangleStrip(InDeltasT const & deltas, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                         OutPointsT & points);
}  // namespace coding

namespace serial
{
class GeometryCodingParams
{
public:
  GeometryCodingParams();
  GeometryCodingParams(uint8_t coordBits, m2::PointD const & pt);
  GeometryCodingParams(uint8_t coordBits, uint64_t basePointUint64);

  m2::PointU GetBasePoint() const { return m_BasePoint; }
  uint64_t GetBasePointUint64() const { return m_BasePointUint64; }
  int64_t GetBasePointInt64() const { return static_cast<int64_t>(m_BasePointUint64); }

  void SetBasePoint(m2::PointD const & pt);

  uint8_t GetCoordBits() const { return m_CoordBits; }

  template <typename WriterT>
  void Save(WriterT & writer) const
  {
    WriteVarUint(writer, GetCoordBits());
    WriteVarUint(writer, m_BasePointUint64);
  }

  template <typename SourceT>
  void Load(SourceT & src)
  {
    uint32_t const coordBits = ReadVarUint<uint32_t>(src);
    ASSERT_LESS(coordBits, 32, ());
    *this = GeometryCodingParams(coordBits, ReadVarUint<uint64_t>(src));
  }

private:
  uint64_t m_BasePointUint64;
  m2::PointU m_BasePoint;
  uint8_t m_CoordBits;
};

namespace pts
{
using PointsU = buffer_vector<m2::PointU, 32>;

m2::PointU D2U(m2::PointD const & p, uint32_t coordBits);

m2::PointD U2D(m2::PointU const & p, uint32_t coordBits);

m2::PointU GetMaxPoint(GeometryCodingParams const & params);

m2::PointU GetBasePoint(GeometryCodingParams const & params);
}  // namespace pts

/// @name Encode and Decode function types.
typedef void (*EncodeFunT)(coding::InPointsT const &, m2::PointU const &, m2::PointU const &, coding::OutDeltasT &);
typedef void (*DecodeFunT)(coding::InDeltasT const &, m2::PointU const &, m2::PointU const &, coding::OutPointsT &);

using DeltasT = buffer_vector<uint64_t, 32>;
using OutPointsT = buffer_vector<m2::PointD, 32>;

void Encode(EncodeFunT fn, std::vector<m2::PointD> const & points, GeometryCodingParams const & params,
            DeltasT & deltas);

/// @name Overloads for different out container types.
void Decode(DecodeFunT fn, DeltasT const & deltas, GeometryCodingParams const & params, OutPointsT & points,
            size_t reserveF = 1);
void Decode(DecodeFunT fn, DeltasT const & deltas, GeometryCodingParams const & params,
            std::vector<m2::PointD> & points, size_t reserveF = 1);

template <class TDecodeFun, class TOutPoints>
void DecodeImpl(TDecodeFun fn, DeltasT const & deltas, GeometryCodingParams const & params, TOutPoints & points,
                size_t reserveF)
{
  size_t const count = deltas.size() * reserveF;

  pts::PointsU upoints;
  upoints.resize(count);

  coding::OutPointsT adapt(upoints);
  (*fn)(make_read_adapter(deltas), pts::GetBasePoint(params), pts::GetMaxPoint(params), adapt);

  if (points.size() < 2)
  {
    // Do not call reserve when loading triangles - they are accumulated to one vector.
    points.reserve(count);
  }

  std::transform(upoints.begin(), upoints.begin() + adapt.size(), std::back_inserter(points),
                 std::bind(&pts::U2D, std::placeholders::_1, params.GetCoordBits()));
}

template <class TSink>
void SavePoint(TSink & sink, m2::PointD const & pt, GeometryCodingParams const & cp)
{
  WriteVarUint(sink, coding::EncodePointDeltaAsUint(PointDToPointU(pt, cp.GetCoordBits()), cp.GetBasePoint()));
}

template <class TSource>
m2::PointD LoadPoint(TSource & src, GeometryCodingParams const & cp)
{
  m2::PointD const pt = PointUToPointD(coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(src), cp.GetBasePoint()),
                                       cp.GetCoordBits());
  return pt;
}

template <class TSink>
void SaveInner(EncodeFunT fn, std::vector<m2::PointD> const & points, GeometryCodingParams const & params, TSink & sink)
{
  DeltasT deltas;
  Encode(fn, points, params, deltas);
  WriteVarUintArray(deltas, sink);
}

template <class TSink>
void WriteBufferToSink(std::vector<char> const & buffer, TSink & sink)
{
  uint32_t const count = static_cast<uint32_t>(buffer.size());
  WriteVarUint(sink, count);
  sink.Write(&buffer[0], count);
}

template <class TSink>
void SaveOuter(EncodeFunT fn, std::vector<m2::PointD> const & points, GeometryCodingParams const & params, TSink & sink)
{
  DeltasT deltas;
  Encode(fn, points, params, deltas);

  std::vector<char> buffer;
  MemWriter<std::vector<char>> writer(buffer);
  WriteVarUintArray(deltas, writer);

  WriteBufferToSink(buffer, sink);
}

void const * LoadInner(DecodeFunT fn, void const * pBeg, size_t count, GeometryCodingParams const & params,
                       OutPointsT & points);

template <class TSource, class TPoints>
void LoadOuter(DecodeFunT fn, TSource & src, GeometryCodingParams const & params, TPoints & points, size_t reserveF = 1)
{
  uint32_t const count = ReadVarUint<uint32_t>(src);
  std::vector<char> buffer(count);
  char * p = &buffer[0];
  src.Read(p, count);

  DeltasT deltas;
  deltas.reserve(count / 2);
  ReadVarUint64Array(p, p + count, base::MakeBackInsertFunctor(deltas));

  Decode(fn, deltas, params, points, reserveF);
}

/// @name Paths.
template <class TSink>
void SaveInnerPath(std::vector<m2::PointD> const & points, GeometryCodingParams const & params, TSink & sink)
{
  SaveInner(&coding::EncodePolyline, points, params, sink);
}

template <class TSink>
void SaveOuterPath(std::vector<m2::PointD> const & points, GeometryCodingParams const & params, TSink & sink)
{
  SaveOuter(&coding::EncodePolyline, points, params, sink);
}

inline void const * LoadInnerPath(void const * pBeg, size_t count, GeometryCodingParams const & params,
                                  OutPointsT & points)
{
  return LoadInner(&coding::DecodePolyline, pBeg, count, params, points);
}

template <class TSource, class TPoints>
void LoadOuterPath(TSource & src, GeometryCodingParams const & params, TPoints & points)
{
  LoadOuter(&coding::DecodePolyline, src, params, points);
}

/// @name Triangles.
template <class TSink>
void SaveInnerTriangles(std::vector<m2::PointD> const & points, GeometryCodingParams const & params, TSink & sink)
{
  SaveInner(&coding::EncodeTriangleStrip, points, params, sink);
}

inline void StripToTriangles(size_t count, OutPointsT const & strip, OutPointsT & triangles)
{
  CHECK_GREATER_OR_EQUAL(count, 2, ());
  triangles.clear();
  triangles.reserve((count - 2) * 3);
  for (size_t i = 2; i < count; ++i)
  {
    triangles.push_back(strip[i - 2]);
    triangles.push_back(strip[i - 1]);
    triangles.push_back(strip[i]);
  }
}

inline void const * LoadInnerTriangles(void const * pBeg, size_t count, GeometryCodingParams const & params,
                                       OutPointsT & triangles)
{
  CHECK_GREATER_OR_EQUAL(count, 2, ());
  OutPointsT points;
  void const * res = LoadInner(&coding::DecodeTriangleStrip, pBeg, count, params, points);

  StripToTriangles(count, points, triangles);
  return res;
}

void DecodeTriangles(coding::InDeltasT const & deltas, m2::PointU const & basePoint, m2::PointU const & maxPoint,
                     coding::OutPointsT & triangles);

template <class TSource>
void LoadOuterTriangles(TSource & src, GeometryCodingParams const & params, OutPointsT & triangles)
{
  uint32_t const count = ReadVarUint<uint32_t>(src);

  for (uint32_t i = 0; i < count; ++i)
    LoadOuter(&DecodeTriangles, src, params, triangles, 3);
}

class TrianglesChainSaver
{
  using TPoint = m2::PointU;
  using TEdge = tesselator::Edge;
  using TBuffer = std::vector<char>;

  TPoint m_base;
  TPoint m_max;

  std::list<TBuffer> m_buffers;

public:
  explicit TrianglesChainSaver(GeometryCodingParams const & params);

  TPoint GetBasePoint() const { return m_base; }
  TPoint GetMaxPoint() const { return m_max; }

  void operator()(TPoint arr[3], std::vector<TEdge> edges);

  size_t GetBufferSize() const
  {
    size_t sz = 0;
    for (auto const & i : m_buffers)
      sz += i.size();
    return sz;
  }

  template <class TSink>
  void Save(TSink & sink)
  {
    // Not necessary assumption that 3-bytes varuint
    // is enough for triangle chains count.
    size_t const count = m_buffers.size();
    CHECK_LESS_OR_EQUAL(count, 0x1FFFFF, ());

    WriteVarUint(sink, static_cast<uint32_t>(count));

    std::for_each(m_buffers.begin(), m_buffers.end(),
                  std::bind(&WriteBufferToSink<TSink>, std::placeholders::_1, std::ref(sink)));
  }
};
}  // namespace serial
