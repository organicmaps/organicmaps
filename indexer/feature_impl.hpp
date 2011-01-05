#pragma once

#include "cell_id.hpp"

#include "../coding/write_to_sink.hpp"
#include "../coding/varint.hpp"

#include "../geometry/point2d.hpp"

namespace feature
{
  namespace detail
  {
    struct pt_2_id
    {
      int64_t operator() (m2::PointD const & p) const
      {
        return PointToInt64(p.x, p.y);
      }
    };
  }

  template <class TSink>
  void SerializePoints(vector<m2::PointD> const & points, TSink & sink)
  {
    uint32_t const ptsCount = points.size();
    ASSERT_GREATER_OR_EQUAL(ptsCount, 1, ());

    vector<int64_t> geom;
    geom.reserve(ptsCount);
    transform(points.begin(), points.end(), back_inserter(geom), detail::pt_2_id());

    if (ptsCount == 1)
    {
      WriteVarInt(sink, geom[0]);
    }
    else
    {
      WriteVarUint(sink, ptsCount - 1);
      for (size_t i = 0; i < ptsCount; ++i)
        WriteVarInt(sink, i == 0 ? geom[0] : geom[i] - geom[i-1]);
    }
  }

  template <class TSink>
  void SerializeTriangles(vector<int64_t> triangles, TSink & sink)
  {
    if (!triangles.empty())
    {
      ASSERT_EQUAL(triangles.size() % 3, 0, (triangles.size()));
      WriteVarUint(sink, triangles.size() / 3 - 1);
      for (size_t i = 0; i < triangles.size(); ++i)
        WriteVarInt(sink, i == 0 ? triangles[i] : (triangles[i] - triangles[i-1]));
    }
  }

  static int g_arrScales[] = { 5, 10, 14, 17 };  // 17 = scales::GetUpperScale()
}
