#pragma once

#include "geometry_coding.hpp"

#include "../geometry/point2d.hpp"

#include "../coding/writer.hpp"
#include "../coding/varint.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/stl_add.hpp"


namespace serial
{
  template <class T, class TSink>
  inline void WriteVarUintArray(vector<T> const & v, TSink & sink)
  {
    for (size_t i = 0; i != v.size(); ++i)
      WriteVarUint(sink, v[i]);
  }

  typedef void (*EncodeFunT)(vector<m2::PointU> const &, m2::PointU const &, m2::PointU const &, vector<uint64_t> &);
  typedef void (*DecodeFunT)(vector<uint64_t> const &, m2::PointU const &, m2::PointU const &, vector<m2::PointU> &);

  void Encode(EncodeFunT fn, vector<m2::PointD> const & points, int64_t base, vector<uint64_t> & deltas);

  typedef buffer_vector<m2::PointD, 32> OutPointsT;
  void Decode(DecodeFunT fn, vector<uint64_t> const & deltas, int64_t base, OutPointsT & points);

  template <class TSink>
  void SaveInner(EncodeFunT fn, vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    vector<uint64_t> deltas;
    Encode(fn, points, base, deltas);
    WriteVarUintArray(deltas, sink);
  }

  template <class TSink>
  void SaveOuter(EncodeFunT fn, vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    vector<uint64_t> deltas;
    Encode(fn, points, base, deltas);

    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    WriteVarUintArray(deltas, writer);

    uint32_t const count = buffer.size();
    WriteVarUint(sink, count);
    sink.Write(&buffer[0], count);
  }

  void const * LoadInner(DecodeFunT fn, void const * pBeg, size_t count, int64_t base, OutPointsT & points);

  template <class TSource>
  void LoadOuter(DecodeFunT fn, TSource & src, int64_t base, OutPointsT & points)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);
    vector<char> buffer(count);
    char * p = &buffer[0];
    src.Read(p, count);

    vector<uint64_t> deltas;
    deltas.reserve(count / 2);
    ReadVarUint64Array(p, p + count, MakeBackInsertFunctor(deltas));

    Decode(fn, deltas, base, points);
  }


  /// @name Pathes.
  //@{
  template <class TSink>
  void SaveInnerPath(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    SaveInner(&geo_coding::EncodePolyline, points, base, sink);
  }
  template <class TSink>
  void SaveOuterPath(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    SaveOuter(&geo_coding::EncodePolyline, points, base, sink);
  }

  inline void const * LoadInnerPath(void const * pBeg, size_t count, int64_t base, OutPointsT & points)
  {
    return LoadInner(&geo_coding::DecodePolyline, pBeg, count, base, points);
  }

  template <class TSource>
  void LoadOuterPath(TSource & src, int64_t base, OutPointsT & points)
  {
    LoadOuter(&geo_coding::DecodePolyline, src, base, points);
  }
  //@}

  /// @name Triangles.
  //@{
  template <class TSink>
  void SaveInnerTriangles(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    SaveInner(&geo_coding::EncodeTriangleStrip, points, base, sink);
  }

  inline void const * LoadInnerTriangles(void const * pBeg, size_t count, int64_t base, OutPointsT & points)
  {
    return LoadInner(&geo_coding::DecodeTriangleStrip, pBeg, count, base, points);
  }
  //@}
}
