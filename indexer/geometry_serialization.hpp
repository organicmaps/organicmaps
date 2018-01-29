#pragma once

#include "indexer/geometry_coding.hpp"
#include "indexer/tesselator_decl.hpp"
#include "indexer/coding_params.hpp"

#include "geometry/point2d.hpp"

#include "coding/point_to_integer.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/buffer_vector.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/list.hpp"


namespace serial
{
  template <class TCont, class TSink>
  inline void WriteVarUintArray(TCont const & v, TSink & sink)
  {
    for (size_t i = 0; i != v.size(); ++i)
      WriteVarUint(sink, v[i]);
  }

  /// @name Encode and Decode function types.
  //@{
  typedef void (*EncodeFunT)(geo_coding::InPointsT const &,
                             m2::PointU const &, m2::PointU const &,
                             geo_coding::OutDeltasT &);
  typedef void (*DecodeFunT)(geo_coding::InDeltasT const &,
                             m2::PointU const &, m2::PointU const &,
                             geo_coding::OutPointsT &);
  //@}

  typedef buffer_vector<uint64_t, 32> DeltasT;
  typedef buffer_vector<m2::PointD, 32> OutPointsT;

  void Encode(EncodeFunT fn, vector<m2::PointD> const & points, CodingParams const & params,
              DeltasT & deltas);

  /// @name Overloads for different out container types.
  //@{
  void Decode(DecodeFunT fn, DeltasT const & deltas, CodingParams const & params,
              OutPointsT & points, size_t reserveF = 1);
  void Decode(DecodeFunT fn, DeltasT const & deltas, CodingParams const & params,
              vector<m2::PointD> & points, size_t reserveF = 1);
  //@}

  template <class TSink>
  void SavePoint(TSink & sink, m2::PointD const & pt, CodingParams const & cp)
  {
    WriteVarUint(sink, EncodeDelta(PointD2PointU(pt, cp.GetCoordBits()), cp.GetBasePoint()));
  }

  template <class TSource>
  m2::PointD LoadPoint(TSource & src, CodingParams const & cp)
  {
    m2::PointD const pt = PointU2PointD(
              DecodeDelta(ReadVarUint<uint64_t>(src), cp.GetBasePoint()), cp.GetCoordBits());
    return pt;
  }

  template <class TSink>
  void SaveInner(EncodeFunT fn, vector<m2::PointD> const & points,
                 CodingParams const & params, TSink & sink)
  {
    DeltasT deltas;
    Encode(fn, points, params, deltas);
    WriteVarUintArray(deltas, sink);
  }

  template <class TSink>
  void WriteBufferToSink(vector<char> const & buffer, TSink & sink)
  {
    uint32_t const count = static_cast<uint32_t>(buffer.size());
    WriteVarUint(sink, count);
    sink.Write(&buffer[0], count);
  }

  template <class TSink>
  void SaveOuter(EncodeFunT fn, vector<m2::PointD> const & points,
                 CodingParams const & params, TSink & sink)
  {
    DeltasT deltas;
    Encode(fn, points, params, deltas);

    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    WriteVarUintArray(deltas, writer);

    WriteBufferToSink(buffer, sink);
  }

  void const * LoadInner(DecodeFunT fn, void const * pBeg, size_t count,
                         CodingParams const & params, OutPointsT & points);

  template <class TSource, class TPoints>
  void LoadOuter(DecodeFunT fn, TSource & src, CodingParams const & params,
                 TPoints & points, size_t reserveF = 1)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);
    vector<char> buffer(count);
    char * p = &buffer[0];
    src.Read(p, count);

    DeltasT deltas;
    deltas.reserve(count / 2);
    ReadVarUint64Array(p, p + count, MakeBackInsertFunctor(deltas));

    Decode(fn, deltas, params, points, reserveF);
  }


  /// @name Paths.
  //@{
  template <class TSink>
  void SaveInnerPath(vector<m2::PointD> const & points, CodingParams const & params, TSink & sink)
  {
    SaveInner(&geo_coding::EncodePolyline, points, params, sink);
  }
  template <class TSink>
  void SaveOuterPath(vector<m2::PointD> const & points, CodingParams const & params, TSink & sink)
  {
    SaveOuter(&geo_coding::EncodePolyline, points, params, sink);
  }

  inline void const * LoadInnerPath(void const * pBeg, size_t count, CodingParams const & params,
                                    OutPointsT & points)
  {
    return LoadInner(&geo_coding::DecodePolyline, pBeg, count, params, points);
  }

  template <class TSource, class TPoints>
  void LoadOuterPath(TSource & src, CodingParams const & params, TPoints & points)
  {
    LoadOuter(&geo_coding::DecodePolyline, src, params, points);
  }
  //@}

  /// @name Triangles.
  //@{
  template <class TSink>
  void SaveInnerTriangles(vector<m2::PointD> const & points,
                          CodingParams const & params, TSink & sink)
  {
    SaveInner(&geo_coding::EncodeTriangleStrip, points, params, sink);
  }

  inline void const * LoadInnerTriangles(void const * pBeg, size_t count,
                                         CodingParams const & params, OutPointsT & triangles)
  {
    CHECK_GREATER_OR_EQUAL(count, 2, ());
    triangles.clear();
    OutPointsT points;
    void const * res = LoadInner(&geo_coding::DecodeTriangleStrip, pBeg, count, params, points);

    triangles.reserve((count - 2) * 3);
    for (size_t i = 2; i < count; ++i)
    {
      triangles.push_back(points[i - 2]);
      triangles.push_back(points[i - 1]);
      triangles.push_back(points[i]);
    }
    return res;
  }

  class TrianglesChainSaver
  {
    using TPoint = m2::PointU;
    using TEdge = tesselator::Edge;
    using TBuffer = vector<char>;

    TPoint m_base;
    TPoint m_max;

    list<TBuffer> m_buffers;

  public:
    explicit TrianglesChainSaver(CodingParams const & params);

    TPoint GetBasePoint() const { return m_base; }
    TPoint GetMaxPoint() const { return m_max; }

    void operator() (TPoint arr[3], vector<TEdge> edges);

    size_t GetBufferSize() const
    {
      size_t sz = 0;
      for (auto const & i : m_buffers)
        sz += i.size();
      return sz;
    }

    template <class TSink> void Save(TSink & sink)
    {
      // Not necessary assumption that 3-bytes varuint
      // is enough for triangle chains count.
      size_t const count = m_buffers.size();
      CHECK_LESS_OR_EQUAL(count, 0x1FFFFF, ());

      WriteVarUint(sink, static_cast<uint32_t>(count));

      for_each(m_buffers.begin(), m_buffers.end(), bind(&WriteBufferToSink<TSink>, _1, ref(sink)));
    }
  };

  void DecodeTriangles(geo_coding::InDeltasT const & deltas,
                       m2::PointU const & basePoint,
                       m2::PointU const & maxPoint,
                       geo_coding::OutPointsT & triangles);

  template <class TSource>
  void LoadOuterTriangles(TSource & src, CodingParams const & params, OutPointsT & triangles)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);

    for (uint32_t i = 0; i < count; ++i)
      LoadOuter(&DecodeTriangles, src, params, triangles, 3);
  }
  //@}
}
