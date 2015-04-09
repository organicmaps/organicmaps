#pragma once

#include "geometry/point2d.hpp"

#include "coding/varint.hpp"

#include "base/base.hpp"
#include "base/bits.hpp"
#include "base/array_adapters.hpp"

//@{
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
//@}


/// Predict next point for polyline with given previous points (p1, p2).
m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2);

/// Predict next point for polyline with given previous points (p1, p2, p3).
m2::PointU PredictPointInPolyline(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2,
                                  m2::PointU const & p3);

/// Predict point for neighbour triangle with given
/// previous triangle (p1, p2, p3) and common edge (p1, p2).
m2::PointU PredictPointInTriangle(m2::PointU const & maxPoint,
                                  m2::PointU const & p1,
                                  m2::PointU const & p2,
                                  m2::PointU const & p3);

/// Geometry Coding-Decoding functions.
namespace geo_coding
{
  typedef array_read<m2::PointU> InPointsT;
  typedef array_write<m2::PointU> OutPointsT;

  typedef array_read<uint64_t> InDeltasT;
  typedef array_write<uint64_t> OutDeltasT;

void EncodePolylinePrev1(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodePolylinePrev1(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolylinePrev2(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodePolylinePrev2(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);

void EncodePolylinePrev3(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodePolylinePrev3(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);

inline void EncodePolyline(InPointsT const & points,
                           m2::PointU const & basePoint,
                           m2::PointU const & maxPoint,
                           OutDeltasT & deltas)
{
  EncodePolylinePrev2(points, basePoint, maxPoint, deltas);
}

inline void DecodePolyline(InDeltasT const & deltas,
                           m2::PointU const & basePoint,
                           m2::PointU const & maxPoint,
                           OutPointsT & points)
{
  DecodePolylinePrev2(deltas, basePoint, maxPoint, points);
}

void EncodeTriangleStrip(InPointsT const & points,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutDeltasT & deltas);

void DecodeTriangleStrip(InDeltasT const & deltas,
                         m2::PointU const & basePoint,
                         m2::PointU const & maxPoint,
                         OutPointsT & points);
}
