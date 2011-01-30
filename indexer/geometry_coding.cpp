#include "geometry_coding.hpp"
#include "../coding/byte_stream.hpp"
#include "../base/assert.hpp"
#include "../base/stl_add.hpp"
#include "../std/complex.hpp"
#include "../std/vector.hpp"

namespace
{

inline void EncodeVarUints(vector<uint64_t> const & varints, vector<char> & serialOutput)
{
  PushBackByteSink<vector<char> > sink(serialOutput);
  for (vector<uint64_t>::const_iterator it = varints.begin(); it != varints.end(); ++it)
    WriteVarUint(sink, *it);
}

template <typename T>
inline m2::PointU ClampPoint(m2::PointU const & maxPoint, m2::Point<T> point)
{
  return m2::PointU(
        point.x < 0 ? 0 : (point.x < maxPoint.x ? static_cast<uint32_t>(point.x) : maxPoint.x),
        point.y < 0 ? 0 : (point.y < maxPoint.y ? static_cast<uint32_t>(point.y) : maxPoint.y));
}

bool TestDecoding(vector<m2::PointU> const & points,
                  m2::PointU const & basePoint,
                  m2::PointU const & maxPoint,
                  vector<char> & serialOutput,
                  void (* fnDecode)(char const * pBeg, char const * pEnd,
                                    m2::PointU const & basePoint,
                                    m2::PointU const & maxPoint,
                                    vector<m2::PointU> & points))
{
  vector<m2::PointU> decoded;
  decoded.reserve(points.size());
  fnDecode(&serialOutput[0], &serialOutput[0] + serialOutput.size(), basePoint, maxPoint, decoded);
  ASSERT_EQUAL(points, decoded, (basePoint, maxPoint));
  return true;
}

}

m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2)
{
  return ClampPoint(maxPoint, m2::PointI64(p1) + m2::PointI64(p1) - m2::PointI64(p2));
}

m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2,
                                  m2::PointU const & p3)
{
  CHECK_NOT_EQUAL(p2, p3, ());

  // In complex numbers:
  // Ci = Ci-1 + (Ci-1 - Ci-2) * (Ci-1 - Ci-2) / (Ci-2 - Ci-3)
  complex<double> const c1(p1.x, p1.y);
  complex<double> const c2(p2.x, p2.y);
  complex<double> const c3(p3.x, p3.y);
  complex<double> const c0 = c1 + (c1 - c2) * (c1 - c2) / (c2 - c3);
  return ClampPoint(maxPoint, m2::PointD(c0.real(), c0.imag()));
}

void EncodePolylinePrev1(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & /*maxPoint*/,
                         vector<char> & serialOutput)
{
  vector<uint64_t> deltas;
  deltas.reserve(points.size());
  if (points.size() > 0)
  {
    deltas.push_back(EncodeDelta(points[0], basePoint));
    for (size_t i = 1; i < points.size(); ++i)
      deltas.push_back(EncodeDelta(points[i], points[i-1]));
  }

  EncodeVarUints(deltas, serialOutput);
  ASSERT(TestDecoding(points, basePoint, m2::PointU(), serialOutput, &DecodePolylinePrev1), ());
}

void DecodePolylinePrev1(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & /*maxPoint*/,
                         vector<m2::PointU> & points)
{
  vector<uint64_t> deltas;
  ReadVarUint64Array(pBeg, pEnd, MakeBackInsertFunctor(deltas));
  points.reserve(points.size() + deltas.size());

  if (deltas.size() > 0)
  {
    points.push_back(DecodeDelta(deltas[0], basePoint));
    for (size_t i = 1; i < deltas.size(); ++i)
      points.push_back(DecodeDelta(deltas[i], points.back()));
  }
}

void EncodePolylinePrev2(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput)
{
  vector<uint64_t> deltas;
  deltas.reserve(points.size());
  if (points.size() > 0)
  {
    deltas.push_back(EncodeDelta(points[0], basePoint));
    if (points.size() > 1)
    {
      deltas.push_back(EncodeDelta(points[1], points[0]));
      for (size_t i = 2; i < points.size(); ++i)
        deltas.push_back(EncodeDelta(points[i],
                                     PredictPointInPolyline(maxPoint, points[i-1], points[i-2])));
    }
  }

  EncodeVarUints(deltas, serialOutput);
  ASSERT(TestDecoding(points, basePoint, maxPoint, serialOutput, &DecodePolylinePrev2), ());
}

void DecodePolylinePrev2(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points)
{
  vector<uint64_t> deltas;
  ReadVarUint64Array(pBeg, pEnd, MakeBackInsertFunctor(deltas));
  points.reserve(points.size() + deltas.size());

  if (deltas.size() > 0)
  {
    points.push_back(DecodeDelta(deltas[0], basePoint));
    if (deltas.size() > 1)
    {
      points.push_back(DecodeDelta(deltas[1], points.back()));
      for (size_t i = 2; i < deltas.size(); ++i)
      {
        size_t const n = points.size();
        points.push_back(DecodeDelta(deltas[i],
                                     PredictPointInPolyline(maxPoint, points[n-1], points[n-2])));
      }
    }
  }
}


void EncodePolylinePrev3(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput)
{
  ASSERT_LESS_OR_EQUAL(basePoint.x, maxPoint.x, (basePoint, maxPoint));
  ASSERT_LESS_OR_EQUAL(basePoint.y, maxPoint.y, (basePoint, maxPoint));

  vector<uint64_t> deltas;
  deltas.reserve(points.size());
  if (points.size() > 0)
  {
    deltas.push_back(EncodeDelta(points[0], basePoint));
    if (points.size() > 1)
    {
      deltas.push_back(EncodeDelta(points[1], points[0]));
      if (points.size() > 2)
      {
        m2::PointU const prediction = PredictPointInPolyline(maxPoint, points[1], points[0]);
        deltas.push_back(EncodeDelta(points[2], prediction));
        for (size_t i = 3; i < points.size(); ++i)
        {
          m2::PointU const prediction =
              PredictPointInPolyline(maxPoint, points[i-1], points[i-2], points[i-3]);
          deltas.push_back(EncodeDelta(points[i], prediction));
        }
      }
    }
  }
  EncodeVarUints(deltas, serialOutput);
  ASSERT(TestDecoding(points, basePoint, maxPoint, serialOutput, &DecodePolylinePrev3), ());
}

void DecodePolylinePrev3(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points)
{
  ASSERT_LESS_OR_EQUAL(basePoint.x, maxPoint.x, (basePoint, maxPoint));
  ASSERT_LESS_OR_EQUAL(basePoint.y, maxPoint.y, (basePoint, maxPoint));

  vector<uint64_t> deltas;
  ReadVarUint64Array(pBeg, pEnd, MakeBackInsertFunctor(deltas));
  points.reserve(points.size() + deltas.size());

  if (deltas.size() > 0)
  {
    points.push_back(DecodeDelta(deltas[0], basePoint));
    if (deltas.size() > 1)
    {
      m2::PointU const pt0 = points.back();
      points.push_back(DecodeDelta(deltas[1], pt0));
      if (deltas.size() > 2)
      {
        points.push_back(DecodeDelta(deltas[2],
                                     PredictPointInPolyline(maxPoint, points.back(), pt0)));
        for (size_t i = 3; i < deltas.size(); ++i)
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
