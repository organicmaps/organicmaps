#include "indexer/geometry_coding.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include "std/complex.hpp"
#include "std/vector.hpp"


namespace
{
  inline m2::PointU ClampPoint(m2::PointU const & maxPoint, m2::Point<double> const & point)
  {
    typedef m2::PointU::value_type uvalue_t;
    //return m2::PointU(my::clamp(static_cast<uvalue_t>(point.x), static_cast<uvalue_t>(0), maxPoint.x),
    //                  my::clamp(static_cast<uvalue_t>(point.y), static_cast<uvalue_t>(0), maxPoint.y));

    return m2::PointU(static_cast<uvalue_t>(my::clamp(point.x, 0.0, static_cast<double>(maxPoint.x))),
                      static_cast<uvalue_t>(my::clamp(point.y, 0.0, static_cast<double>(maxPoint.y))));
  }
}

m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2)
{
  // return ClampPoint(maxPoint, m2::PointI64(p1) + m2::PointI64(p1) - m2::PointI64(p2));
  // return ClampPoint(maxPoint, m2::PointI64(p1) + (m2::PointI64(p1) - m2::PointI64(p2)) / 2);
  return ClampPoint(maxPoint, m2::PointD(p1) + (m2::PointD(p1) - m2::PointD(p2)) / 2.0);
}

m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2,
                                  m2::PointU const & p3)
{
  CHECK_NOT_EQUAL(p2, p3, ());

  complex<double> const c1(p1.x, p1.y);
  complex<double> const c2(p2.x, p2.y);
  complex<double> const c3(p3.x, p3.y);
  complex<double> const d = (c1 - c2) / (c2 - c3);
  complex<double> const c0 = c1 + (c1 - c2) * polar(0.5, 0.5 * arg(d));

  /*
  complex<double> const c1(p1.x, p1.y);
  complex<double> const c2(p2.x, p2.y);
  complex<double> const c3(p3.x, p3.y);
  complex<double> const d = (c1 - c2) / (c2 - c3);
  complex<double> const c01 = c1 + (c1 - c2) * polar(0.5, arg(d));
  complex<double> const c02 = c1 + (c1 - c2) * complex<double>(0.5, 0.0);
  complex<double> const c0 = (c01 + c02) * complex<double>(0.5, 0.0);
  */

  return ClampPoint(maxPoint, m2::PointD(c0.real(), c0.imag()));
}

m2::PointU PredictPointInTriangle(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2,
                                  m2::PointU const & p3)
{
  // parallelogram prediction
  return ClampPoint(maxPoint, p1 + p2 - p3);
}


namespace geo_coding
{
  bool TestDecoding(InPointsT const & points,
                    m2::PointU const & basePoint,
                    m2::PointU const & maxPoint,
                    OutDeltasT const & deltas,
                    void (* fnDecode)(InDeltasT const & deltas,
                                      m2::PointU const & basePoint,
                                      m2::PointU const & maxPoint,
                                      OutPointsT & points))
  {
    size_t const count = points.size();

    vector<m2::PointU> decoded;
    decoded.resize(count);

    OutPointsT decodedA(decoded);
    fnDecode(make_read_adapter(deltas), basePoint, maxPoint, decodedA);

    for (size_t i = 0; i < count; ++i)
      ASSERT_EQUAL(points[i], decoded[i], ());
    return true;
  }

void EncodePolylinePrev1(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas)
{
  size_t const count = points.size();
  if (count > 0)
  {
    deltas.push_back(EncodeDelta(points[0], basePoint));
    for (size_t i = 1; i < count; ++i)
      deltas.push_back(EncodeDelta(points[i], points[i-1]));
  }

  ASSERT(TestDecoding(points, basePoint, maxPoint, deltas, &DecodePolylinePrev1), ());
}

void DecodePolylinePrev1(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & /*maxPoint*/,
                         OutPointsT & points)
{
  size_t const count = deltas.size();
  if (count > 0)
  {
    points.push_back(DecodeDelta(deltas[0], basePoint));
    for (size_t i = 1; i < count; ++i)
      points.push_back(DecodeDelta(deltas[i], points.back()));
  }
}

void EncodePolylinePrev2(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas)
{
  size_t const count = points.size();
  if (count > 0)
  {
    deltas.push_back(EncodeDelta(points[0], basePoint));
    if (count > 1)
    {
      deltas.push_back(EncodeDelta(points[1], points[0]));
      for (size_t i = 2; i < count; ++i)
        deltas.push_back(EncodeDelta(points[i],
                                     PredictPointInPolyline(maxPoint, points[i-1], points[i-2])));
    }
  }

  ASSERT(TestDecoding(points, basePoint, maxPoint, deltas, &DecodePolylinePrev2), ());
}

void DecodePolylinePrev2(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points)
{
  size_t const count = deltas.size();
  if (count > 0)
  {
    points.push_back(DecodeDelta(deltas[0], basePoint));
    if (count > 1)
    {
      points.push_back(DecodeDelta(deltas[1], points.back()));
      for (size_t i = 2; i < count; ++i)
      {
        size_t const n = points.size();
        points.push_back(DecodeDelta(deltas[i],
                                     PredictPointInPolyline(maxPoint, points[n-1], points[n-2])));
      }
    }
  }
}

void EncodePolylinePrev3(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas)
{
  ASSERT_LESS_OR_EQUAL(basePoint.x, maxPoint.x, (basePoint, maxPoint));
  ASSERT_LESS_OR_EQUAL(basePoint.y, maxPoint.y, (basePoint, maxPoint));

  size_t const count = points.size();
  if (count > 0)
  {
    deltas.push_back(EncodeDelta(points[0], basePoint));
    if (count > 1)
    {
      deltas.push_back(EncodeDelta(points[1], points[0]));
      if (count > 2)
      {
        m2::PointU const prediction = PredictPointInPolyline(maxPoint, points[1], points[0]);
        deltas.push_back(EncodeDelta(points[2], prediction));
        for (size_t i = 3; i < count; ++i)
        {
          m2::PointU const prediction =
              PredictPointInPolyline(maxPoint, points[i-1], points[i-2], points[i-3]);
          deltas.push_back(EncodeDelta(points[i], prediction));
        }
      }
    }
  }

  ASSERT(TestDecoding(points, basePoint, maxPoint, deltas, &DecodePolylinePrev3), ());
}

void DecodePolylinePrev3(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points)
{
  ASSERT_LESS_OR_EQUAL(basePoint.x, maxPoint.x, (basePoint, maxPoint));
  ASSERT_LESS_OR_EQUAL(basePoint.y, maxPoint.y, (basePoint, maxPoint));

  size_t const count = deltas.size();
  if (count> 0)
  {
    points.push_back(DecodeDelta(deltas[0], basePoint));
    if (count > 1)
    {
      m2::PointU const pt0 = points.back();
      points.push_back(DecodeDelta(deltas[1], pt0));
      if (count > 2)
      {
        points.push_back(DecodeDelta(deltas[2],
                                     PredictPointInPolyline(maxPoint, points.back(), pt0)));
        for (size_t i = 3; i < count; ++i)
        {
          size_t const n = points.size();
          m2::PointU const prediction =
              PredictPointInPolyline(maxPoint, points[n-1], points[n-2], points[n-3]);
          points.push_back(DecodeDelta(deltas[i], prediction));
        }
      }
    }
  }
}

void EncodeTriangleStrip(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas)
{
  size_t const count = points.size();
  if (count > 0)
  {
    ASSERT_GREATER(count, 2, ());

    deltas.push_back(EncodeDelta(points[0], basePoint));
    deltas.push_back(EncodeDelta(points[1], points[0]));
    deltas.push_back(EncodeDelta(points[2], points[1]));

    for (size_t i = 3; i < count; ++i)
    {
      m2::PointU const prediction =
          PredictPointInTriangle(maxPoint, points[i-1], points[i-2], points[i-3]);
      deltas.push_back(EncodeDelta(points[i], prediction));
    }
  }
}

void DecodeTriangleStrip(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points)
{
  size_t const count = deltas.size();
  if (count > 0)
  {
    ASSERT_GREATER(count, 2, ());

    points.push_back(DecodeDelta(deltas[0], basePoint));
    points.push_back(DecodeDelta(deltas[1], points.back()));
    points.push_back(DecodeDelta(deltas[2], points.back()));

    for (size_t i = 3; i < count; ++i)
    {
      size_t const n = points.size();
      m2::PointU const prediction =
          PredictPointInTriangle(maxPoint, points[n-1], points[n-2], points[n-3]);
      points.push_back(DecodeDelta(deltas[i], prediction));
    }
  }
}

}
