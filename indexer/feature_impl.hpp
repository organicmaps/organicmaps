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
    void WriteCellsSimple(vector<int64_t> & cells, TSink & sink)
    {
      for (size_t i = 0; i < cells.size(); ++i)
        WriteVarInt(sink, i == 0 ? cells[0] : cells[i] - cells[i-1]);
    }

    template <class TSink>
    void WriteCells(vector<int64_t> & cells, TSink & sink)
    {
      vector<char> buffer;
      MemWriter<vector<char> > writer(buffer);

      WriteCellsSimple(cells, writer);

      uint32_t const count = static_cast<uint32_t>(buffer.size());
      WriteVarUint(sink, count);
      sink.Write(&buffer[0], count);
    }

    class points_emitter
    {
      vector<m2::PointD> & m_points;
      int64_t m_id;

    public:
      points_emitter(vector<m2::PointD> & points, uint32_t count)
        : m_points(points), m_id(0)
      {
        m_points.reserve(count);
      }
      void operator() (int64_t id)
      {
        m_points.push_back(pts::ToPoint(m_id += id));
      }
    };

    inline void const * ReadPointsSimple( void const * p, size_t count,
                                          vector<m2::PointD> & points)
    {
      return ReadVarInt64Array(p, count, points_emitter(points, count));
    }

    template <class TSource>
    void ReadPoints(vector<m2::PointD> & points, TSource & src)
    {
      uint32_t const count = ReadVarUint<uint32_t>(src);
      vector<char> buffer(count);
      char * p = &buffer[0];
      src.Read(p, count);

      ReadVarInt64Array(p, p + count, points_emitter(points, count / 2));
    }
  }

  template <class TSink>
  void SavePointsSimple(vector<m2::PointD> const & points, TSink & sink)
  {
    ASSERT_GREATER ( points.size(), 1, () );

    vector<int64_t> cells;
    detail::TransformPoints(points, cells);

    detail::WriteCellsSimple(cells, sink);
  }

  template <class TSink>
  void SavePoints(vector<m2::PointD> const & points, TSink & sink)
  {
    ASSERT_GREATER ( points.size(), 1, () );

    vector<int64_t> cells;
    detail::TransformPoints(points, cells);

    detail::WriteCells(cells, sink);
  }

  inline void const * LoadPointsSimple( void const * p, size_t count,
                                        vector<m2::PointD> & points)
  {
    ASSERT_GREATER ( count, 1, () );
    void const * ret = detail::ReadPointsSimple(p, count, points);
    ASSERT_GREATER ( points.size(), 1, () );
    return ret;
  }

  template <class TSource>
  void LoadPoints(vector<m2::PointD> & points, TSource & src)
  {
    detail::ReadPoints(points, src);

    ASSERT_GREATER ( points.size(), 1, () );
  }

  template <class TSink>
  void SaveTriangles(vector<m2::PointD> const & triangles, TSink & sink)
  {
    uint32_t const count = triangles.size();
    ASSERT_GREATER ( count, 0, () );
    ASSERT_EQUAL ( count % 3, 0, (count) );

    vector<int64_t> cells;
    detail::TransformPoints(triangles, cells);

    detail::WriteCells(cells, sink);
  }

  template <class TSource>
  void LoadTriangles(vector<m2::PointD> & points, TSource & src)
  {
    detail::ReadPoints(points, src);

    uint32_t const count = points.size();
    ASSERT_GREATER ( count, 0, () );
    ASSERT_EQUAL ( count % 3, 0, (count) );
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
