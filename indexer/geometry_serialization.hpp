#pragma once

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

  void EncodePath(vector<m2::PointD> const & points, int64_t base, vector<uint64_t> & deltas);

  typedef buffer_vector<m2::PointD, 32> OutPointsT;
  void DecodePath(vector<uint64_t> const & deltas, int64_t base, OutPointsT & points);

  template <class TSink>
  void SaveInnerPath(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    vector<uint64_t> deltas;
    EncodePath(points, base, deltas);
    WriteVarUintArray(deltas, sink);
  }

  template <class TSink>
  void SaveOuterPath(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    vector<uint64_t> deltas;
    EncodePath(points, base, deltas);

    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    WriteVarUintArray(deltas, writer);

    uint32_t const count = buffer.size();
    WriteVarUint(sink, count);
    sink.Write(&buffer[0], count);
  }

  void const * LoadInnerPath(void const * pBeg, size_t count, int64_t base, OutPointsT & points);

  template <class TSource>
  void LoadOuterPath(TSource & src, int64_t base, OutPointsT & points)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);
    vector<char> buffer(count);
    char * p = &buffer[0];
    src.Read(p, count);

    vector<uint64_t> deltas;
    deltas.reserve(count / 2);
    ReadVarUint64Array(p, p + count, MakeBackInsertFunctor(deltas));

    DecodePath(deltas, base, points);
  }
}
