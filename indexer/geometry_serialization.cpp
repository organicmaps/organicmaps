#include "geometry_serialization.hpp"
#include "mercator.hpp"
#include "point_to_int64.hpp"
#include "geometry_coding.hpp"

#include "../geometry/pointu_to_uint64.hpp"

#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"


namespace serial
{
  namespace pts
  {
    inline m2::PointU D2U(m2::PointD const & p)
    {
      return PointD2PointU(p.x, p.y);
    }

    inline m2::PointD U2D(m2::PointU const & p)
    {
      CoordPointT const pt = PointU2PointD(p);
      return m2::PointD(pt.first, pt.second);
    }

    inline m2::PointU GetMaxPoint()
    {
      return D2U(m2::PointD(MercatorBounds::maxX, MercatorBounds::maxY));
    }

    inline m2::PointU GetBasePoint(int64_t base)
    {
      return m2::Uint64ToPointU(base);
    }
  }

  void Encode(EncodeFunT fn, vector<m2::PointD> const & points, int64_t base, vector<uint64_t> & deltas)
  {
    vector<m2::PointU> upoints;
    upoints.reserve(points.size());

    transform(points.begin(), points.end(), back_inserter(upoints), &pts::D2U);

    (*fn)(upoints, pts::GetBasePoint(base), pts::GetMaxPoint(), deltas);
  }

  void Decode(DecodeFunT fn, vector<uint64_t> const & deltas, int64_t base, OutPointsT & points)
  {
    vector<m2::PointU> upoints;
    upoints.reserve(deltas.size());

    (*fn)(deltas, pts::GetBasePoint(base), pts::GetMaxPoint(), upoints);

    points.reserve(upoints.size());
    transform(upoints.begin(), upoints.end(), back_inserter(points), &pts::U2D);
  }

  void const * LoadInner(DecodeFunT fn, void const * pBeg, size_t count, int64_t base, OutPointsT & points)
  {
    vector<uint64_t> deltas;
    deltas.reserve(count);
    void const * ret = ReadVarUint64Array(static_cast<char const *>(pBeg), count,
                                          MakeBackInsertFunctor(deltas));

    Decode(fn, deltas, base, points);
    return ret;
  }
}
