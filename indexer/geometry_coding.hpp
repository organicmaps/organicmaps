#pragma once

#include "../geometry/point2d.hpp"
#include "../base/base.hpp"
#include "../std/vector.hpp"
#include "../std/tuple.hpp"

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
