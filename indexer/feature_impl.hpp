#pragma once

#include "cell_id.hpp"

#include "../coding/write_to_sink.hpp"
#include "../coding/varint.hpp"

#include "../geometry/point2d.hpp"

namespace feature
{
  namespace pts
  {
    inline int64_t FromPoint(m2::PointD const & p)
    {
      return PointToInt64(p.x, p.y);
    }

    inline m2::PointD ToPoint(int64_t i)
    {
      CoordPointT const pt = Int64ToPoint(i);
      return m2::PointD(pt.first, pt.second);
    }
  }

  namespace detail
  {
    inline void TransformPoints(vector<m2::PointD> const & points, vector<int64_t> & cells)
    {
      cells.reserve(points.size());
      transform(points.begin(), points.end(), back_inserter(cells), &pts::FromPoint);
    }

    template <class TSink>
    void WriteCellsSimple(vector<int64_t> const & cells, int64_t base, TSink & sink)
    {
      for (size_t i = 0; i < cells.size(); ++i)
        WriteVarInt(sink, i == 0 ? cells[0] - base : cells[i] - cells[i-1]);
    }

    template <class TSink>
    void WriteCells(vector<int64_t> const & cells, int64_t base, TSink & sink)
    {
      vector<char> buffer;
      MemWriter<vector<char> > writer(buffer);

      WriteCellsSimple(cells, base, writer);

      uint32_t const count = static_cast<uint32_t>(buffer.size());
      WriteVarUint(sink, count);
      sink.Write(&buffer[0], count);
    }

    template <class TCont> class points_emitter
    {
      TCont & m_points;
      int64_t m_id;

    public:
      points_emitter(TCont & points, uint32_t count, int64_t base)
        : m_points(points), m_id(base)
      {
        m_points.reserve(count);
      }
      void operator() (int64_t id)
      {
        m_points.push_back(pts::ToPoint(m_id += id));
      }
    };

    template <class TCont>
    void const * ReadPointsSimple(void const * p, size_t count, int64_t base, TCont & points)
    {
      return ReadVarInt64Array(p, count, points_emitter<TCont>(points, count, base));
    }

    template <class TCont, class TSource>
    void ReadPoints(TCont & points, int64_t base, TSource & src)
    {
      uint32_t const count = ReadVarUint<uint32_t>(src);
      vector<char> buffer(count);
      char * p = &buffer[0];
      src.Read(p, count);

      ReadVarInt64Array(p, p + count, points_emitter<TCont>(points, count / 2, base));
    }
  }

  template <class TSink>
  void SavePointsSimple(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    ASSERT_GREATER ( points.size(), 1, () );

    vector<int64_t> cells;
    detail::TransformPoints(points, cells);

    detail::WriteCellsSimple(cells, base, sink);
  }

  template <class TSink>
  void SavePoints(vector<m2::PointD> const & points, int64_t base, TSink & sink)
  {
    ASSERT_GREATER ( points.size(), 1, () );

    vector<int64_t> cells;
    detail::TransformPoints(points, cells);

    detail::WriteCells(cells, base, sink);
  }

  template <class TCont>
  void const * LoadPointsSimple(void const * p, size_t count, int64_t base, TCont & points)
  {
    ASSERT_GREATER ( count, 1, () );
    void const * ret = detail::ReadPointsSimple(p, count, base, points);
    ASSERT_GREATER ( points.size(), 1, () );
    return ret;
  }

  template <class TCont, class TSource>
  void LoadPoints(TCont & points, int64_t base, TSource & src)
  {
    detail::ReadPoints(points, base, src);

    ASSERT_GREATER ( points.size(), 1, () );
  }

  template <class TSink>
  void SaveTriangles(vector<m2::PointD> const & triangles, int64_t base, TSink & sink)
  {
#ifdef DEBUG
    uint32_t const count = triangles.size();
    ASSERT_GREATER ( count, 0, () );
    ASSERT_EQUAL ( count % 3, 0, (count) );
#endif

    vector<int64_t> cells;
    detail::TransformPoints(triangles, cells);

    detail::WriteCells(cells, base, sink);
  }

  template <class TCont, class TSource>
  void LoadTriangles(TCont & points, int64_t base, TSource & src)
  {
    detail::ReadPoints(points, base, src);

#ifdef DEBUG
    uint32_t const count = points.size();
    ASSERT_GREATER ( count, 0, () );
    ASSERT_EQUAL ( count % 3, 0, (count) );
#endif
  }


  static int g_arrScales[] = { 7, 10, 14, 17 };  // 17 = scales::GetUpperScale()

  inline string GetTagForIndex(char const * prefix, int ind)
  {
    string str;
    str.reserve(strlen(prefix) + 1);
    str = prefix;

    static char arrChar[] = { '0', '1', '2', '3' };
    STATIC_ASSERT ( ARRAY_SIZE(arrChar) == ARRAY_SIZE(g_arrScales) );
    ASSERT ( ind >= 0 && ind < ARRAY_SIZE(arrChar), (ind) );

    str += arrChar[ind];
    return str;
  }
}
