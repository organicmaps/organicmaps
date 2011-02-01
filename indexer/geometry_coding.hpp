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

namespace geo_coding
{
  typedef vector<m2::PointU> InPointsT;
  typedef vector<m2::PointU> OutPointsT;
  typedef vector<uint64_t> DeltasT;

void EncodePolylinePrev1(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         DeltasT & deltas);

void DecodePolylinePrev1(DeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolylinePrev2(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         DeltasT & deltas);

void DecodePolylinePrev2(DeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolylinePrev3(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         DeltasT & deltas);

void DecodePolylinePrev3(DeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);

inline void EncodePolyline(InPointsT const & points,
                           m2::PointU const & basePoint,
                           m2::PointU const & maxPoint,
                           DeltasT & deltas)
{
  EncodePolylinePrev2(points, basePoint, maxPoint, deltas);
}

inline void DecodePolyline(DeltasT const & deltas,
                           m2::PointU const & basePoint,
                           m2::PointU const & maxPoint,
                           OutPointsT & points)
{
  DecodePolylinePrev2(deltas, basePoint, maxPoint, points);
}

typedef vector<tuple<uint32_t, uint32_t, uint32_t> > TrianglesT;

void EncodeTriangles(InPointsT const & points,
                     TrianglesT const & triangles,
                     m2::PointU const & basePoint,
                     m2::PointU const & maxPoint,
                     DeltasT & deltas);

void DecodeTriangles(DeltasT const & deltas,
                     m2::PointU const & basePoint,
                     m2::PointU const & maxPoint,
                     OutPointsT & points,
                     TrianglesT & triangles);

void EncodeTriangleStrip(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         DeltasT & deltas);

void DecodeTriangleStrip(DeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);
}
