#pragma once

#include "../geometry/point2d.hpp"
#include "../coding/varint.hpp"
#include "../base/base.hpp"
#include "../base/bits.hpp"
#include "../std/vector.hpp"
#include "../std/tuple.hpp"

inline uint64_t EncodeDelta(m2::PointU const & actual, m2::PointU const & prediction)
{
  return bits::BitwiseMerge(
        bits::ZigZagEncode(static_cast<int32_t>(actual.x) - static_cast<int32_t>(prediction.x)),
        bits::ZigZagEncode(static_cast<int32_t>(actual.y) - static_cast<int32_t>(prediction.y)));
}

inline m2::PointU DecodeDelta(uint64_t delta, m2::PointU const & prediction)
{
  uint32_t x, y;
  bits::BitwiseSplit(delta, x, y);
  return m2::PointU(prediction.x + bits::ZigZagDecode(x), prediction.y + bits::ZigZagDecode(y));
}

// Predict point p0 given previous (p1, p2).
m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2);

// Predict point p0 given previous (p1, p2, p3).
m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2,
                                  m2::PointU const & p3);


void EncodePolylinePrev1(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput);

void DecodePolylinePrev1(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points);

void EncodePolylinePrev2(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput);

void DecodePolylinePrev2(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points);

void EncodePolylinePrev3(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput);

void DecodePolylinePrev3(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points);

inline void EncodePolyline(vector<m2::PointU> const & points,
                           m2::PointU const & basePoint,
                           m2::PointU const & maxPoint,
                           vector<char> & serialOutput)
{
  EncodePolylinePrev3(points, basePoint, maxPoint, serialOutput);
}

inline void DecodePolyline(char const * pBeg, char const * pEnd,
                           m2::PointU const & basePoint,
                           m2::PointU const & maxPoint,
                           vector<m2::PointU> & points)
{
  DecodePolylinePrev3(pBeg, pEnd, basePoint, maxPoint, points);
}

void EncodeTriangles(vector<m2::PointU> const & points,
                     vector<tuple<uint32_t, uint32_t, uint32_t> > const & triangles,
                     m2::PointU const & basePoint,
                     m2::PointU const & maxPoint,
                     vector<char> & serialOutput);

void DecodeTriangles(char const * pBeg, char const * pEnd,
                     m2::PointU const & basePoint,
                     m2::PointU const & maxPoint,
                     vector<m2::PointU> & points,
                     vector<tuple<uint32_t, uint32_t, uint32_t> > & triangles);

void EncodeTriangleStrip(vector<m2::PointU> const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<char> & serialOutput);

void DecodeTriangleStrip(char const * pBeg, char const * pEnd,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         vector<m2::PointU> & points);
